#include "fcservice.h"
#include <QtConcurrent>
#include <QImage>
#include "fcutil.h"
#include "fcmuxer.h"
#include "fcfilter.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

FCService::FCService()
{
	qRegisterMetaType<QList<AVStream*>>();
	qRegisterMetaType<QList<AVFrame*>>();

	_threadPool = new QThreadPool(this);
	_threadPool->setExpiryTimeout(-1);
	_threadPool->setMaxThreadCount(1);
}

FCService::~FCService()
{
	destroy();
}

void FCService::openFileAsync(const QString& filePath)
{
	QMutexLocker _(&_mutex);

	_demuxer = new FCDemuxer();

	QtConcurrent::run(_threadPool, [&, filePath]() {
		QMutexLocker _(&_mutex);
		QTime time;
		time.start();
		_lastError = _demuxer->open(filePath);
		qDebug() << "open file time " << time.elapsed();
		if (_lastError < 0)
		{
			delete _demuxer;
			_demuxer = nullptr;
			emit errorOcurred();
		}
		else
		{
			emit fileOpened(_demuxer->streams());
		}
		});
}

void FCService::decodeOnePacketAsync(int streamIndex)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		auto [err, frames] = _demuxer->decodeNextPacket({ streamIndex });
		_lastError = err;
		if (_lastError < 0)
		{
			emit errorOcurred();
		}
		else
		{
			emit frameDeocded(frames);
			emit decodeFinished();
		}
		});
}

void FCService::decodePacketsAsync(int streamIndex, int count)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		for (int i = 0; i < count;)
		{
			auto [err, frames] = _demuxer->decodeNextPacket({ streamIndex });
			if (_lastError = err; err < 0)
			{
				emit errorOcurred();
				return;
			}
			++i;
			emit frameDeocded(frames);
		}
		emit decodeFinished();
		});
}

AVFormatContext* FCService::formatContext() const
{
	return _demuxer->formatContext();
}

AVStream* FCService::stream(int streamIndex) const
{
	return _demuxer->stream(streamIndex);
}

QList<AVStream*> FCService::streams() const
{
	return _demuxer->streams();
}

void FCService::seekAsync(int streamIndex, double seconds)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		if (_demuxer->fastSeek(streamIndex, _demuxer->secondToTs(streamIndex, seconds)) < 0)
		{
			emit errorOcurred();
		}
		else
		{
			emit seekFinished();
		}
		});
}

void FCService::scaleAsync(AVFrame *frame, int destWidth, int destHeight)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		QImage image(destWidth, destHeight, QImage::Format_RGB888);
		auto scaleResult = scale(frame, AV_PIX_FMT_RGB24, destWidth, destHeight);
		if (scaleResult.first)
		{
			for (int i = 0; i < destHeight; ++i)
			{
				memcpy(image.scanLine(i), scaleResult.first[0] + i * scaleResult.second[0], destWidth * (image.depth() / 8));
			}
		}
		emit scaleFinished(QPixmap::fromImage(image));
		});
}

void FCService::saveAsync(const FCMuxEntry &entry)
{
	FCMuxEntry *muxEntry = new FCMuxEntry(entry);

	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		// 按视频流 seek 到后边的关键帧
		_demuxer->fastSeek(muxEntry->vStreamIndex, muxEntry->startPts);

		auto demuxStream = _demuxer->stream(muxEntry->vStreamIndex);
		if (muxEntry->fps <= 0)
		{
			muxEntry->fps = demuxStream->avg_frame_rate.num / demuxStream->avg_frame_rate.den;
		}

		FCMuxer muxer;
		_lastError = muxer.create(*muxEntry);
		if (_lastError < 0)
		{
			emit errorOcurred();
			return;
		}
		auto muxStream = muxer.videoStream();
		FCFilter filter;
		auto filterStr = QString("scale=width=960:height=540,fps=fps=%1").arg(muxEntry->fps).toStdString();
		_lastError = filter.create(filterStr.data(), muxEntry->width, muxEntry->height, muxer.videoFormat(), demuxStream->time_base, demuxStream->sample_aspect_ratio);
		if (_lastError < 0)
		{
			emit errorOcurred();
			return;
		}

		AVFrame *scaledFrame = av_frame_alloc();
		scaledFrame->width = muxEntry->width;
		scaledFrame->height = muxEntry->height;
		scaledFrame->format = muxer.videoFormat();
		int scaledBytes = av_image_get_buffer_size((AVPixelFormat)scaledFrame->format, muxEntry->width, muxEntry->height, 1);
		uint8_t *scaledData = (uint8_t *)av_malloc(scaledBytes);
		_lastError = av_image_fill_arrays(scaledFrame->data, scaledFrame->linesize, scaledData, (AVPixelFormat)scaledFrame->format, muxEntry->width, muxEntry->height, 1);
		if (_lastError < 0)
		{
			emit errorOcurred();
			return;
		}
		_lastError = av_frame_make_writable(scaledFrame);
		if (_lastError < 0)
		{
			FCUtil::printAVError(_lastError, "av_frame_make_writable");
			_lastError = 0;
// 			emit errorOcurred();
// 			return;
		}

		int64_t pts = 0;
		int64_t endPts = muxEntry->startPts + muxEntry->duration * (demuxStream->time_base.den / demuxStream->time_base.num);
		int count = muxEntry->duration;
		if (muxEntry->durationUnit == DURATION_SECOND)
		{
			count = INT_MAX;
		}
		while (count > 0 && _lastError >= 0)
		{
			auto [err, decodedFrames] = _demuxer->decodeNextPacket({ muxEntry->vStreamIndex });
			_lastError = err;
			for (int i = 0; i < decodedFrames.size() && _lastError >= 0; ++i)
			{
				auto decodedFrame = decodedFrames[i];
				if (decodedFrame->pts < muxEntry->startPts)
				{
					continue;
				}
				if (muxEntry->durationUnit == DURATION_SECOND && decodedFrame->pts > endPts)
				{
					count = 0;
					break;
				}

				scaledFrame->pts = decodedFrame->pts;
				auto scaleResult = scale(decodedFrame, muxer.videoFormat(), muxEntry->width, muxEntry->height, scaledFrame->data, scaledFrame->linesize);
				if (_lastError < 0)
				{
					break;
				}

				auto [err, filteredFrames] = filter.filter(scaledFrame);
				if (_lastError = err; _lastError < 0)
				{
					break;
				}
				for (int i = 0; i < filteredFrames.size() && _lastError >= 0; ++i)
				{
					auto filteredFrame = filteredFrames[i];
					filteredFrame->pts = pts++;
					_lastError = muxer.writeVideo(filteredFrame);
					if (_lastError < 0)
					{
						break;
					}
					count -= _lastError;
				}
			}
			for (auto& frame : decodedFrames)
			{
				av_frame_free(&frame);
			}
		}
		if (_lastError >= 0)
		{
			_lastError = muxer.writeTrailer();
		}

		av_frame_free(&scaledFrame);
		delete muxEntry;

		if (_lastError)
		{
			emit errorOcurred();
		}
		else
		{
			emit saveFinished();
		}
		});
}

QPair<int, QString> FCService::lastError()
{
	char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
	av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, _lastError);
	_lastErrorString = QString::fromLocal8Bit(buf);
	return { _lastError, _lastErrorString };
}

double FCService::timestampToSecond(int streamIndex, int64_t timestamp)
{
	return _demuxer->tsToSecond(streamIndex, timestamp);
}

void FCService::destroy()
{
	QMutexLocker _(&_mutex);
	if (_demuxer)
	{
		_demuxer->close();
		delete _demuxer;
		_demuxer = nullptr;
	}
	if (_threadPool)
	{
		delete _threadPool;
		_threadPool = nullptr;
	}
	_vecScaler.clear();
}

FCScaler::ScaleResult FCService::scale(AVFrame *frame, AVPixelFormat destFormat, int destWidth, int destHeight, uint8_t *scaledData[4], int scaledLineSizes[4])
{
	FCScaler::ScaleResult result = { nullptr, nullptr };
	do 
	{
		auto scaler = getScaler(frame, destWidth, destHeight, destFormat);
		if (!scaler)
		{
			break;
		}
		result = scaler->scale(frame->data, frame->linesize, scaledData, scaledLineSizes);
		if (!result.first)
		{
			_lastError = scaler->lastError();
			break;
		}
	} while (0);
	return result;
}

QSharedPointer<FCScaler> FCService::getScaler(AVFrame *frame, int destWidth, int destHeight, AVPixelFormat destFormat)
{
	if (!_vecScaler.isEmpty())
	{
		for (auto& i : _vecScaler)
		{
			if (i->equal(frame->width, frame->height, (AVPixelFormat)frame->format, destWidth, destHeight, destFormat))
			{
				return i;
			}
		}
	}
	QSharedPointer<FCScaler> scaler = QSharedPointer<FCScaler>(new FCScaler());
	_lastError = scaler->create(frame->width, frame->height, (AVPixelFormat)frame->format, destWidth, destHeight, destFormat);
	if (_lastError < 0)
	{
		return {};
	}
	_vecScaler.push_back(scaler);
	return scaler;
}
#include "fcservice.h"
#include <QtConcurrent>
#include <QImage>
#include "fcutil.h"
#include "fcmuxer.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

FCService::FCService()
{
	qRegisterMetaType<QList<AVStream*>>();
	qRegisterMetaType<QList<FCFrame>>();

	_threadPool = new QThreadPool(this);
	_threadPool->setExpiryTimeout(-1);
	_threadPool->setMaxThreadCount(1);

	av_log_set_level(AV_LOG_TRACE);
	av_log_set_callback(&av_log_default_callback);
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
		if (_lastError == AVERROR_EOF)
		{
			_lastError = 0;
			emit eof();
		}
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
			_lastError = err;
			if (_lastError == AVERROR_EOF)
			{
				_lastError = 0;
				emit eof();
				break;
			}
			if (_lastError < 0)
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
		if (_demuxer->fastSeek(streamIndex, _demuxer->secToTs(streamIndex, seconds)) < 0)
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

void FCService::saveAsync(const FCMuxEntry &muxEntry)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		auto entry = muxEntry;
		QMutexLocker _(&_mutex);
		// 按视频流 seek 到后边的关键帧
		auto startPts = _demuxer->secToTs(entry.vStreamIndex, entry.startSec);
		auto endPts = _demuxer->secToTs(entry.vStreamIndex, entry.startSec + entry.durationSec);
		_demuxer->fastSeek(entry.vStreamIndex, startPts);

		auto demuxVideoStream = _demuxer->stream(entry.vStreamIndex);
		if (entry.fps <= 0)
		{
			entry.fps = demuxVideoStream->avg_frame_rate.num / demuxVideoStream->avg_frame_rate.den;
		}

		FCMuxer muxer;
		_lastError = muxer.create(entry);
		if (_lastError < 0)
		{
			emit errorOcurred();
			return;
		}
		QVector<int> streamFilter;
		if (muxer.videoStream())
		{
			streamFilter.push_back(entry.vStreamIndex);
		}
		if (muxer.audioStream())
		{
			streamFilter.push_back(entry.aStreamIndex);
		}
		if (streamFilter.isEmpty())
		{
			qCritical("choose at least one stream to save!");
			return;
		}

		auto videoFilter = createVideoFilter(demuxVideoStream, entry.filterString, muxer.videoFormat());
		if (_lastError < 0)
		{
			emit errorOcurred();
			return;
		}

		bool ending = false;
		while (!ending && _lastError >= 0)
		{
			auto [err, decodedFrames] = _demuxer->decodeNextPacket(streamFilter);
			_lastError = err;
			ending = _lastError == AVERROR_EOF;
			if (ending)
			{
				_lastError = 0;
				for (auto i : streamFilter) // 尾部放一个空帧，flush filter & encoder 用
				{
					decodedFrames.push_back({ i, nullptr });
				}
			}
			for (int i = 0; i < decodedFrames.size() && _lastError >= 0; ++i)
			{
				auto decodedFrame = decodedFrames[i];
				if (decodedFrame.frame && decodedFrame.streamIndex == entry.vStreamIndex)
				{
					if (decodedFrame.frame->pts < startPts)
					{
						continue;
					}
					if (decodedFrame.frame->pts > endPts)
					{
						ending = true;
						for (auto i : streamFilter) // 尾部放一个空帧，flush filter & encoder 用
						{
							decodedFrames.push_back({ i, nullptr });
						}
					}
				}

				if (entry.vStreamIndex == decodedFrame.streamIndex)
				{
					auto [err, filteredFrames] = videoFilter->filter(decodedFrame.frame);
					if (_lastError = err; _lastError < 0)
					{
						clearFrames(filteredFrames);
						break;
					}
					if (!decodedFrame.frame) // flush encoder
					{
						filteredFrames.push_back(nullptr);
					}
					_lastError = muxer.writeVideos(filteredFrames);
					clearFrames(filteredFrames);
				}
				else
				{
					_lastError = muxer.writeAudio(decodedFrame.frame);
				}
			}
			clearFrames(decodedFrames);
		}
		if (_lastError >= 0)
		{
			_lastError = muxer.writeTrailer();
		}
		if (_lastError < 0)
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

double FCService::tsToSec(int streamIndex, int64_t timestamp)
{
	return _demuxer->tsToSec(streamIndex, timestamp);
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

void FCService::clearFrames(QList<AVFrame *> &frames)
{
	for (auto &f : frames)
	{
		av_frame_free(&f);
	}
	frames.clear();
}

void FCService::clearFrames(QList<FCFrame> &frames)
{
	for (auto &f : frames)
	{
		av_frame_free(&(f.frame));
	}
	frames.clear();
}

QSharedPointer<FCFilter> FCService::createVideoFilter(const AVStream *srcStream, QString filters, AVPixelFormat dstPixelFormat)
{
	QSharedPointer<FCFilter> filter(new FCFilter());
	if (!filters.isEmpty())
	{
		filters.append(',');
	}
	filters.append("format=").append(av_get_pix_fmt_name(dstPixelFormat));
	auto strFilter = filters.toStdString();
	_lastError = filter->create(srcStream->codecpar->width, srcStream->codecpar->height, (AVPixelFormat)srcStream->codecpar->format, srcStream->time_base, srcStream->sample_aspect_ratio, strFilter.data(), dstPixelFormat);
	return filter;
}
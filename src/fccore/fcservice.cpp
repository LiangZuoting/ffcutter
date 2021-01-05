#include "fcservice.h"
#include <QtConcurrent>
#include <QImage>
#include <QStringList>
#include "fcutil.h"
#include "fcaudiofilter.h"
#include "fcvideostreamwriter.h"
#include "fcaudiostreamwriter.h"
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

	_demuxer.reset(new FCDemuxer());

	QtConcurrent::run(_threadPool, [&, filePath]() {
		QMutexLocker _(&_mutex);
		QTime time;
		time.start();
		_lastError = _demuxer->open(filePath);
		qDebug() << "open file time " << time.elapsed();
		if (_lastError < 0)
		{
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

void FCService::fastSeekAsync(int streamIndex, double seconds)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		if ((_lastError = _demuxer->fastSeek(streamIndex, _demuxer->secToTs(streamIndex, seconds))) < 0)
		{
			emit errorOcurred();
		}
		else
		{
			emit seekFinished(streamIndex, QList<FCFrame>());
		}
		});
}

void FCService::exactSeekAsync(int streamIndex, double seconds)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		auto [err, frames] = _demuxer->exactSeek(streamIndex, _demuxer->secToTs(streamIndex, seconds));
		_lastError = err;
		if ( _lastError < 0 && _lastError != AVERROR_EOF)
		{
			emit errorOcurred();
		}
		else
		{
			emit seekFinished(streamIndex, frames);
		}
		});
}

void FCService::scaleAsync(AVFrame *frame, int dstWidth, int dstHeight)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		QImage image(dstWidth, dstHeight, QImage::Format_RGB888);
		auto scaleResult = scale(frame, AV_PIX_FMT_RGB24, dstWidth, dstHeight);
		if (scaleResult.first)
		{
			for (int i = 0; i < dstHeight; ++i)
			{
				memcpy(image.scanLine(i), scaleResult.first[0] + i * scaleResult.second[0], dstWidth * (image.depth() / 8));
			}
		}
		emit scaleFinished(frame, QPixmap::fromImage(image));
		});
}

void FCService::saveAsync(const FCMuxEntry &muxEntry)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		auto entry = muxEntry;
		QMutexLocker _(&_mutex);
		// 按视频流 seek 到后边的关键帧
		auto vStartPts = _demuxer->secToTs(entry.vStreamIndex, entry.startSec);
		_demuxer->fastSeek(entry.vStreamIndex, vStartPts);

		AVStream* demuxAudioStream = _demuxer->stream(entry.aStreamIndex);
		auto demuxVideoStream = _demuxer->stream(entry.vStreamIndex);
		if (entry.fps <= 0)
		{
			entry.fps = av_q2d(demuxVideoStream->avg_frame_rate) + 0.5;
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
			streamFilter.append(entry.vStreamIndex);
		}
		if (muxer.audioStream())
		{
			streamFilter.append(entry.aStreamIndex);
		}

		FCVideoStreamWriter vWriter(entry, _demuxer, muxer);
		if (_lastError = vWriter.create(); _lastError < 0)
		{
			emit errorOcurred();
			return;
		}
		FCAudioStreamWriter aWriter(entry, _demuxer, muxer);
		if (_lastError = aWriter.create(); _lastError < 0)
		{
			emit errorOcurred();
			return;
		}

		while ((!vWriter.eof() || !aWriter.eof()) && _lastError >= 0)
		{
			auto [err, decodedFrames] = _demuxer->decodeNextPacket(streamFilter);
			_lastError = err;
			if (_lastError == AVERROR_EOF)
			{
				vWriter.setEof();
				aWriter.setEof();
				_lastError = 0;
				for (auto i : streamFilter) // 尾部放一个空帧，flush filter & encoder 用
				{
					decodedFrames.push_back({ i, nullptr });
				}
			}
			for (int i = 0; i < decodedFrames.size() && _lastError >= 0; ++i)
			{
				auto decodedFrame = decodedFrames[i];
				_lastError = vWriter.write(decodedFrame);
				_lastError = aWriter.write(decodedFrame);
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
		_demuxer.reset();
	}
	if (_threadPool)
	{
		delete _threadPool;
		_threadPool = nullptr;
	}
	_vecScaler.clear();
}

FCScaler::ScaleResult FCService::scale(AVFrame *frame, AVPixelFormat dstFormat, int dstWidth, int dstHeight, uint8_t *scaledData[4], int scaledLineSizes[4])
{
	FCScaler::ScaleResult result = { nullptr, nullptr };
	do 
	{
		auto scaler = getScaler(frame, dstWidth, dstHeight, dstFormat);
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

QSharedPointer<FCScaler> FCService::getScaler(AVFrame *frame, int dstWidth, int dstHeight, AVPixelFormat dstFormat)
{
	if (!_vecScaler.isEmpty())
	{
		for (auto& i : _vecScaler)
		{
			if (i->equal(frame->width, frame->height, (AVPixelFormat)frame->format, dstWidth, dstHeight, dstFormat))
			{
				return i;
			}
		}
	}
	QSharedPointer<FCScaler> scaler = QSharedPointer<FCScaler>(new FCScaler());
	_lastError = scaler->create(frame->width, frame->height, (AVPixelFormat)frame->format, dstWidth, dstHeight, dstFormat);
	if (_lastError < 0)
	{
		return {};
	}
	_vecScaler.push_back(scaler);
	return scaler;
}

void FCService::clearFrames(QList<FCFrame> &frames)
{
	for (auto &f : frames)
	{
		av_frame_free(&(f.frame));
	}
	frames.clear();
}
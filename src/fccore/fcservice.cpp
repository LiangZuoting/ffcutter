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
	QtConcurrent::run(_threadPool, [&, filePath]() {
		QMutexLocker _(&_mutex);
		QTime time;
		time.start();
		auto url = filePath.toStdString();
		do
		{
			_lastError = avformat_open_input(&_formatContext, url.data(), nullptr, nullptr);
			if (_lastError < 0)
			{
				FCUtil::printAVError(_lastError, "avformat_open_input");
				break;
			}
			_lastError = avformat_find_stream_info(_formatContext, nullptr);
			if (_lastError < 0)
			{
				FCUtil::printAVError(_lastError, "avformat_find_stream_info");
				break;
			}
			for (unsigned i = 0; i < _formatContext->nb_streams; ++i)
			{
				auto stream = _formatContext->streams[i];
				_mapFromIndexToStream.insert(stream->index, stream);
			}
		} while (0);
		qDebug() << "open file time " << time.elapsed();

		if (_lastError < 0)
		{
			if (_formatContext)
			{
				avformat_close_input(&_formatContext);
			}
			emit errorOcurred();
		}
		else
		{
			emit fileOpened(_mapFromIndexToStream.values());
		}
		});
}

void FCService::decodeOnePacketAsync(int streamIndex)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		auto frames = decodeNextPacket({ streamIndex });
		emit frameDeocded(frames);
		emit decodeFinished();
		});
}

void FCService::decodePacketsAsync(int streamIndex, int count)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		for (int i = 0; i < count;)
		{
			auto frames = decodeNextPacket({ streamIndex });
			if (auto [err, errStr] = lastError(); err)
			{
				emit errorOcurred();
				return;
			}
			if (!frames.isEmpty())
			{
				++i;
				emit frameDeocded(frames);
			}
		}
		emit decodeFinished();
		});
}

AVFormatContext* FCService::formatContext() const
{
	return _formatContext;
}

AVStream* FCService::stream(int streamIndex) const
{
	if (auto iter = _mapFromIndexToStream.constFind(streamIndex); iter != _mapFromIndexToStream.cend())
	{
		return iter.value();
	}
	return nullptr;
}

QList<AVStream*> FCService::streams() const
{
	return _mapFromIndexToStream.values();
}

void FCService::seekAsync(int streamIndex, double timestampInSecond)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		auto stream = _mapFromIndexToStream[streamIndex];
		int64_t timestamp = timestampInSecond * (stream->time_base.den / stream->time_base.num);
		if (!seek(streamIndex, timestamp))
		{
			emit errorOcurred();
		}
		else
		{
			emit seekFinished();
		}
		});
}

void FCService::scaleAsync(AVFrame *frame, AVPixelFormat destFormat, int destWidth, int destHeight)
{
	QMutexLocker _(&_mutex);
	QtConcurrent::run(_threadPool, [=]() {
		QMutexLocker _(&_mutex);
		QImage image(destWidth, destHeight, QImage::Format_RGB888);
		auto scaleResult = scale(frame, destFormat, destWidth, destHeight);
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
		seek(muxEntry->vStreamIndex, muxEntry->startPts);

		auto stream = _mapFromIndexToStream[muxEntry->vStreamIndex];
		if (muxEntry->fps <= 0)
		{
			muxEntry->fps = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
		}

		FCMuxer muxer;
		_lastError = muxer.create(*muxEntry);
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
		av_frame_make_writable(scaledFrame);

		int64_t pts = 0;
		int64_t endPts = muxEntry->startPts + muxEntry->duration * (stream->time_base.den / stream->time_base.num);
		int count = muxEntry->duration;
		if (muxEntry->durationUnit == DURATION_SECOND)
		{
			count = INT_MAX;
		}
		while (count > 0)
		{
			auto frames = decodeNextPacket({ muxEntry->vStreamIndex });
			if (_lastError < 0)
			{
				break;
			}
			if (frames.isEmpty())
			{
				continue;
			}
			for (auto frame : frames)
			{
				if (frame->pts < muxEntry->startPts)
				{
					continue;
				}
				if (muxEntry->durationUnit == DURATION_SECOND && frame->pts > endPts)
				{
					count = 0;
					break;
				}

				scaledFrame->pts = pts++;
				auto scaleResult = scale(frame, muxer.videoFormat(), muxEntry->width, muxEntry->height, scaledFrame->data, scaledFrame->linesize);
				if (_lastError < 0)
				{
					break;
				}
				_lastError = muxer.writeVideo(scaledFrame);
				if (_lastError < 0)
				{
					break;
				}
			}
			for (auto& frame : frames)
			{
				av_frame_free(&frame);
			}
		}

		_lastError = muxer.writeTrailer();

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
	auto stream = _mapFromIndexToStream[streamIndex];
	return double(timestamp) / (stream->time_base.den / stream->time_base.num);
}

void FCService::destroy()
{
	QMutexLocker _(&_mutex);
	if (auto codecs = _mapFromIndexToCodec.values(); !codecs.isEmpty())
	{
		for (auto& c : codecs)
		{
			avcodec_free_context(&c);
		}
		_mapFromIndexToCodec.clear();
	}
	if (_formatContext)
	{
		avformat_close_input(&_formatContext);
		_mapFromIndexToStream.clear();
	}
	if (_readPacket)
	{
		av_packet_free(&_readPacket);
	}
	if (_threadPool)
	{
		delete _threadPool;
		_threadPool = nullptr;
	}
	_vecScaler.clear();
}

bool FCService::seek(int streamIndex, int64_t timestamp)
{
	_lastError = av_seek_frame(_formatContext, streamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
	if (_lastError < 0)
	{
		FCUtil::printAVError(_lastError, "av_seek_frame, stream:", streamIndex, ",ts:", timestamp);
		return false;
	}
	auto codecContext = getCodecContext(streamIndex);
	if (codecContext)
	{
		avcodec_flush_buffers(codecContext);
	}
	return true;
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

QList<AVFrame*> FCService::decodeNextPacket(const QVector<int> &streamFilter)
{
	QTime time;
	time.start();
	QList<AVFrame*> frames;
	auto packet = getPacket();
	while (_lastError >= 0)
	{
		_lastError = av_read_frame(_formatContext, packet);
		if (_lastError == AVERROR(EAGAIN) || _lastError == AVERROR_EOF)
		{
			_lastError = 0;
			return frames;
		}
		if (_lastError < 0)
		{
			FCUtil::printAVError(_lastError, "av_read_frame");
			break;
		}
		if (!streamFilter.isEmpty() && !streamFilter.contains(packet->stream_index))
		{
			continue;
		}
		auto codecContext = getCodecContext(packet->stream_index);
		if (!codecContext)
		{
			return frames;
		}
		_lastError = avcodec_send_packet(codecContext, packet);
		if (_lastError < 0)
		{
			FCUtil::printAVError(_lastError, "avcodec_send_packet");
			break;
		}
		while (_lastError >= 0)
		{
			AVFrame* frame = av_frame_alloc();
			_lastError = avcodec_receive_frame(codecContext, frame);
			if (!_lastError)
			{
				frames.push_back(frame);
				continue;
			}
			av_frame_free(&frame);
			if (_lastError == AVERROR(EAGAIN) || _lastError == AVERROR_EOF)
			{
				qDebug() << "decode frame time " << time.elapsed();
				_lastError = 0;
				return frames;
			}
			FCUtil::printAVError(_lastError, "avcodec_receive_frame");
			break;
		}
	}
	return frames;
}

AVCodecContext* FCService::getCodecContext(int streamIndex)
{
	if (auto iter = _mapFromIndexToCodec.constFind(streamIndex); iter != _mapFromIndexToCodec.cend())
	{
		return iter.value();
	}
	else if (auto iter = _mapFromIndexToStream.constFind(streamIndex); iter != _mapFromIndexToStream.cend())
	{
		auto stream = iter.value();
		AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
		AVCodecContext* codecContext = avcodec_alloc_context3(codec);
		_lastError = avcodec_parameters_to_context(codecContext, stream->codecpar);
		if (_lastError < 0)
		{
			FCUtil::printAVError(_lastError, "avcodec_parameters_to_context");
			avcodec_free_context(&codecContext);
			return nullptr;
		}
		AVDictionary* opts{};
		_lastError = avcodec_open2(codecContext, codec, &opts);
		if (_lastError < 0)
		{
			FCUtil::printAVError(_lastError, "avcodec_open2");
			avcodec_free_context(&codecContext);
			return nullptr;
		}
		_mapFromIndexToCodec.insert(streamIndex, codecContext);
		return codecContext;
	}
	return nullptr;
}

AVPacket* FCService::getPacket()
{
	if (!_readPacket)
	{
		_readPacket = av_packet_alloc();
	}
	av_init_packet(_readPacket);
	return _readPacket;
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
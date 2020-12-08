#include "fcservice.h"
#include <QtConcurrent>
#include <QImage>
extern "C"
{
#include <libavcodec/avcodec.h>
}

FCService::FCService()
{
	qRegisterMetaType<QList<AVStream*>>();
}

FCService::~FCService()
{
	destroy();
}

void FCService::openFileAsync(const QString& filePath)
{
	QMutexLocker _(&_mutex);
	auto t = getThreadPool(DEMUX_INDEX);
	QtConcurrent::run(t, [&, filePath]() {
		QMutexLocker _(&_mutex);
		QTime time;
		time.start();
		auto url = filePath.toStdString();
		do
		{
			_lastError = avformat_open_input(&_formatContext, url.data(), nullptr, nullptr);
			if (_lastError < 0)
			{
				break;
			}
			_lastError = avformat_find_stream_info(_formatContext, nullptr);
			if (_lastError < 0)
			{
				break;
			}
			for (unsigned i = 0; i < _formatContext->nb_streams; ++i)
			{
				auto stream = _formatContext->streams[i];
				_mapFromIndexToStream.insert(stream->index, stream);
			}
		} while (0);

		if (_lastError && _formatContext)
		{
			avformat_close_input(&_formatContext);
		}
		qDebug() << "open file time " << time.elapsed();
		emit fileOpened(_mapFromIndexToStream.values());
		});
}

void FCService::decodeOneFrameAsync(int streamIndex)
{
	QMutexLocker _(&_mutex);
	auto t = getThreadPool(streamIndex);
	QtConcurrent::run(t, [=]() {
		QMutexLocker _(&_mutex);
		auto frame = decodeNextFrame(streamIndex);
		emit frameDeocded(frame);
		emit decodeFinished();
		});
}

void FCService::decodeFramesAsync(int streamIndex, int count)
{
	QMutexLocker _(&_mutex);
	auto t = getThreadPool(streamIndex);
	QtConcurrent::run(t, [=]() {
		QMutexLocker _(&_mutex);
		for (int i = 0; i < count; ++i)
		{
			auto frame = decodeNextFrame(streamIndex);
			if (!frame)
			{
				break;
			}
			emit frameDeocded(frame);
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

void FCService::seek(int streamIndex, int64_t timestamp)
{
	av_seek_frame(_formatContext, streamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
}

void FCService::scaleAsync(AVFrame *frame, int destWidth, int destHeight)
{
	QtConcurrent::run([=]() {
		QMutexLocker _(&_scaleMutex);
		auto scaler = getScaler(frame, destWidth, destHeight, AV_PIX_FMT_RGB24);
		if (!scaler)
		{
			emit scaleFinished(QPixmap());
			return;
		}
		auto [data, lineSizes] = scaler->scale(frame->data, frame->linesize);
		if (!data)
		{
			emit scaleFinished(QPixmap());
			return;
		}
		QImage image(destWidth, destHeight, QImage::Format_RGB888);
		for (int i = 0; i < destHeight; ++i)
		{
			memcpy(image.scanLine(i), data[0] + i * lineSizes[0], destWidth * (image.depth() / 8));
		}
		emit scaleFinished(QPixmap::fromImage(image));
		});
}

QPair<int, QString> FCService::lastError()
{
	char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
	av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, _lastError);
	_lastErrorString = QString::fromLocal8Bit(buf);
	return { _lastError, _lastErrorString };
}

void FCService::destroy()
{
	{
		QMutexLocker _(&_mutex);
		if (_formatContext)
		{
			avformat_close_input(&_formatContext);
			_mapFromIndexToStream.clear();
		}
		if (auto codecs = _mapFromIndexToCodec.values(); !codecs.isEmpty())
		{
			for (auto c : codecs)
			{
				avcodec_free_context(&c);
			}
			_mapFromIndexToCodec.clear();
		}
		if (auto packets = _mapFromIndexToPacket.values(); !packets.isEmpty())
		{
			for (auto p : packets)
			{
				av_packet_free(&p);
			}
			_mapFromIndexToPacket.clear();
		}
		if (auto threads = _mapFromIndexToThread.values(); !threads.isEmpty())
		{
			for (auto t : threads)
			{
				delete t;
			}
			_mapFromIndexToThread.clear();
		}
	}
	
	{
		QMutexLocker _(&_scaleMutex);
		_vecScaler.clear();
	}
}

AVFrame* FCService::decodeNextFrame(int streamIndex)
{
	QTime time;
	time.start();
	auto codecContext = getCodecContext(streamIndex);
	if (!codecContext)
	{
		return nullptr;
	}
	auto packet = getPacket(streamIndex);
	AVFrame* frame = av_frame_alloc();
	while (true)
	{
		_lastError = av_read_frame(_formatContext, packet);
		if (_lastError < 0)
		{
			break;
		}
		if (packet->stream_index != streamIndex)
		{
			continue;
		}
		_lastError = avcodec_send_packet(codecContext, packet);
		if (_lastError < 0)
		{
			break;
		}
		_lastError = avcodec_receive_frame(codecContext, frame);
		if (!_lastError)
		{
			qDebug() << "decode frame time " << time.elapsed();
			return frame;
		}
		if (_lastError == AVERROR(EAGAIN))
		{
			continue;
		}
		break;
	}
	av_frame_free(&frame);
	return nullptr;
}

QThreadPool* FCService::getThreadPool(int streamIndex)
{
	if (auto iter = _mapFromIndexToThread.constFind(streamIndex); iter != _mapFromIndexToThread.cend())
	{
		return iter.value();
	}
	else
	{
		QThreadPool* t = new QThreadPool();
		t->setExpiryTimeout(-1);
		t->setMaxThreadCount(1);
		_mapFromIndexToThread.insert(streamIndex, t);
		return t;
	}
}

AVCodecContext* FCService::getCodecContext(int streamIndex)
{
	if (auto iter = _mapFromIndexToCodec.constFind(streamIndex); iter != _mapFromIndexToCodec.cend())
	{
		return iter.value();
	}
	else
	{
		auto stream = _mapFromIndexToStream[streamIndex];
		AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
		AVCodecContext* codecContext = avcodec_alloc_context3(codec);
		_lastError = avcodec_parameters_to_context(codecContext, stream->codecpar);
		if (_lastError < 0)
		{
			avcodec_free_context(&codecContext);
			return nullptr;
		}
		AVDictionary* opts{};
		_lastError = avcodec_open2(codecContext, codec, &opts);
		if (_lastError < 0)
		{
			avcodec_free_context(&codecContext);
			return nullptr;
		}
		_mapFromIndexToCodec.insert(streamIndex, codecContext);
		return codecContext;
	}
}

AVPacket* FCService::getPacket(int streamIndex)
{
	if (auto iter = _mapFromIndexToPacket.constFind(streamIndex); iter != _mapFromIndexToPacket.cend())
	{
		auto packet = iter.value();
		av_init_packet(packet);
		return packet;
	}
	else
	{
		AVPacket* packet = av_packet_alloc();
		_mapFromIndexToPacket.insert(streamIndex, packet);
		return packet;
	}
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
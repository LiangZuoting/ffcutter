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

void FCService::seekAsync(int streamIndex, double timestampInSecond)
{
	auto stream = _mapFromIndexToStream[streamIndex];
	int64_t timestamp = timestampInSecond * (stream->time_base.den / stream->time_base.num);
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

void FCService::saveAsync(const FCMuxEntry &entry)
{
	FCMuxEntry *muxEntry = new FCMuxEntry(entry);
	auto stream = _mapFromIndexToStream[muxEntry->vStreamIndex];

	AVFormatContext *ofc = nullptr;
	_lastError = avformat_alloc_output_context2(&ofc, nullptr, nullptr, muxEntry->filePath);

	auto vc = avcodec_find_encoder(ofc->oformat->video_codec);
	auto vcc = avcodec_alloc_context3(vc);
	vcc->time_base = stream->time_base;
	vcc->width = muxEntry->width;
	vcc->height = muxEntry->height;
	vcc->pix_fmt = AV_PIX_FMT_RGB8;
	vcc->bit_rate = 400000;
	vcc->framerate = stream->avg_frame_rate;
	auto vs = avformat_new_stream(ofc, nullptr);
	vs->time_base = vcc->time_base;
	vs->avg_frame_rate = stream->avg_frame_rate;
	vs->r_frame_rate = vs->avg_frame_rate;
	_lastError = avcodec_parameters_from_context(vs->codecpar, vcc);
	_lastError = avcodec_open2(vcc, vc, nullptr);
	{
		char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, _lastError);
		_lastErrorString = QString::fromLocal8Bit(buf);
	}
	_lastError = avio_open(&ofc->pb, muxEntry->filePath, AVIO_FLAG_WRITE);
	_lastError = avformat_write_header(ofc, nullptr);

	//_lastError = av_seek_frame(_formatContext, muxEntry->vStreamIndex, muxEntry->startPts, AVSEEK_FLAG_BACKWARD);
	auto packet = getPacket();
	int count = muxEntry->duration;
	if (muxEntry->durationUnit == DURATION_SECOND)
	{
		count = INT_MAX;
	}
	auto outPacket = av_packet_alloc();
	while (count--)
	{
		auto frame = decodeNextFrame(muxEntry->vStreamIndex);
		if (frame->pts < muxEntry->startPts)
		{
			continue;
		}
		if (muxEntry->durationUnit == DURATION_SECOND && frame->pts > (muxEntry->startPts + muxEntry->duration * (stream->time_base.num / stream->time_base.den)))
		{
			break;
		}

		_lastError = avcodec_send_frame(vcc, frame);
		if (_lastError < 0)
		{
			break;
		}
		while (_lastError >= 0)
		{
			_lastError = avcodec_receive_packet(vcc, outPacket);
			if (_lastError == AVERROR(EAGAIN) || _lastError == AVERROR_EOF)
			{
				_lastError = 0;
				break;
			}
			if (_lastError < 0) 
			{
				{
					char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
					av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, _lastError);
					_lastErrorString = QString::fromLocal8Bit(buf);
				}
			}
			_lastError = av_interleaved_write_frame(ofc, outPacket);
			if (_lastError < 0)
			{
				{
					char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
					av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, _lastError);
					_lastErrorString = QString::fromLocal8Bit(buf);
				}
				assert(0);
			}
		}
	}
	_lastError = av_write_trailer(ofc);

	avcodec_free_context(&vcc);
	avio_closep(&ofc->pb);
	avformat_free_context(ofc);
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
		if (_readPacket)
		{
			av_packet_free(&_readPacket);
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
	auto packet = getPacket();
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
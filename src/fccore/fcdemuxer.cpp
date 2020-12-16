#include "fcdemuxer.h"
#include "fcutil.h"

FCDemuxer::~FCDemuxer()
{
	close();
}

int FCDemuxer::open(const QString& filePath)
{
	int ret = 0;
	auto url = filePath.toStdString();
	do 
	{
		ret = avformat_open_input(&_formatContext, url.data(), nullptr, nullptr);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avformat_open_input");
			break;
		}
		ret = avformat_find_stream_info(_formatContext, nullptr);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avformat_find_stream_info");
			break;
		}

		_demuxedPacket = av_packet_alloc();
		for (unsigned i = 0; i < _formatContext->nb_streams; ++i)
		{
			auto stream = _formatContext->streams[i];
			_streams.insert(stream->index, stream);
		}
	} while (0);
	return ret;
}

int FCDemuxer::fastSeek(int streamIndex, int64_t timestamp)
{
	int ret = av_seek_frame(_formatContext, streamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
	if (ret < 0)
	{
		FCUtil::printAVError(ret, "av_seek_frame, stream:", streamIndex, "timestamp:", timestamp);
	}
	auto [_, codecContext] = getCodecContext(streamIndex);
	if (codecContext)
	{
		avcodec_flush_buffers(codecContext);
	}
	return ret;
}

int FCDemuxer::exactSeek(int streamIndex, int64_t timestamp)
{
	return 0;
}

FCDecodeResult FCDemuxer::decodeNextPacket(const QVector<int>& streamFilter)
{
	int ret = 0;
	QList<FCFrame> frames;
	int streamIndex = 0;
	while (ret >= 0)
	{
		ret = av_read_frame(_formatContext, _demuxedPacket);
		if (ret == AVERROR_EOF) // flush all decoders
		{
			for (auto i : streamFilter)
			{
				auto [err, f] = decodePacket(i, nullptr);
				ret = err;
				frames.append(f);
			}
			return { !ret ? AVERROR_EOF : ret, frames };
		}
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "av_read_frame");
			break;
		}
		streamIndex = _demuxedPacket->stream_index;
		if (!streamFilter.isEmpty() && !streamFilter.contains(_demuxedPacket->stream_index))
		{
			continue;
		}
		auto [err, f] = decodePacket(streamIndex, _demuxedPacket);
		ret = err;
		if (!f.isEmpty())
		{
			frames = f;
			break;
		}
	}
	return { ret, frames };
}

AVFormatContext* FCDemuxer::formatContext() const
{
	return _formatContext;
}

QList<AVStream*> FCDemuxer::streams() const
{
	return _streams.values();
}

AVStream* FCDemuxer::stream(int streamIndex) const
{
	if (auto iter = _streams.constFind(streamIndex); iter != _streams.cend())
	{
		return iter.value();
	}
	return nullptr;
}

double FCDemuxer::tsToSecond(int streamIndex, int64_t timestamp) const
{
	auto tb = stream(streamIndex)->time_base;
	return (double)timestamp / (tb.den / tb.num);
}

int64_t FCDemuxer::secondToTs(int streamIndex, double seconds) const
{
	auto tb = stream(streamIndex)->time_base;
	return seconds * (tb.den / tb.num);
}

void FCDemuxer::close()
{
	if (_formatContext)
	{
		avformat_free_context(_formatContext);
		_formatContext = nullptr;
		_streams.clear();
	}
	for (auto iter = _codecContexts.begin(); iter != _codecContexts.end(); ++iter)
	{
		avcodec_free_context(&(iter.value()));
	}
	_codecContexts.clear();
	if (_demuxedPacket)
	{
		av_packet_unref(_demuxedPacket);
		_demuxedPacket = nullptr;
	}
}

QPair<int, AVCodecContext*> FCDemuxer::getCodecContext(int streamIndex)
{
	if (auto iter = _codecContexts.constFind(streamIndex); iter != _codecContexts.cend())
	{
		return { 0, iter.value() };
	}
	if (auto iter = _streams.constFind(streamIndex); iter != _streams.cend())
	{
		auto stream = iter.value();
		int ret = 0;
		AVCodecContext* codecContext = nullptr;
		do 
		{
			AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
			codecContext = avcodec_alloc_context3(codec);
			ret = avcodec_parameters_to_context(codecContext, stream->codecpar);
			if (ret < 0)
			{
				FCUtil::printAVError(ret, "avcodec_parameters_to_context");
				break;
			}
			ret = avcodec_open2(codecContext, codec, nullptr);
			if (ret < 0)
			{
				FCUtil::printAVError(ret, "avcodec_open2");
				break;
			}
		} while (0);
		if (ret < 0)
		{
			if (codecContext)
			{
				avcodec_free_context(&codecContext);
			}
		}
		else
		{
			_codecContexts.insert(streamIndex, codecContext);
		}
		return { ret, codecContext };
	}
	FCUtil::printAVError(AVERROR_INVALIDDATA, "getCodecContext");
	return { AVERROR_INVALIDDATA, nullptr };
}

FCDecodeResult FCDemuxer::decodePacket(int streamIndex, AVPacket *packet)
{
	int ret = 0;
	QList<FCFrame> frames;
	do 
	{
		auto [err, codecContext] = getCodecContext(streamIndex);
		ret = err;
		if (ret < 0)
		{
			break;
		}
		ret = avcodec_send_packet(codecContext, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			ret = 0;
			break;
		}
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avcodec_send_packet");
		}
		while (ret >= 0)
		{
			AVFrame *frame = av_frame_alloc();
			ret = avcodec_receive_frame(codecContext, frame);
			if (ret >= 0)
			{
				if (frame->pts == AV_NOPTS_VALUE)
				{
					frame->pts = frame->pkt_dts;
				}
				frames.push_back({ streamIndex, frame });
				continue;
			}
			av_frame_free(&frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				ret = 0;
				break;
			}
			FCUtil::printAVError(ret, "avcodec_receive_frame");
		}
	} while (0);
	return { ret, frames };
}
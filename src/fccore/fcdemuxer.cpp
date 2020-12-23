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
	auto [_, decoder] = getCodecContext(streamIndex);
	if (decoder)
	{
		decoder->flushBuffers();
	}
	return ret;
}

FCDecodeResult FCDemuxer::exactSeek(int streamIndex, int64_t timestamp)
{
	int ret = fastSeek(streamIndex, timestamp);
	QList<FCFrame> resultFrames;
	if (ret < 0)
	{
		return {ret, resultFrames};
	}
	while (resultFrames.isEmpty())
	{
		auto [err, frames] = decodeNextPacket({ streamIndex });
		if (ret = err, ret < 0)
		{
			break;
		}
		while (!frames.isEmpty())
		{
			auto &frame = frames.front();
			auto pts = frame.frame->pts;
			if (pts == AV_NOPTS_VALUE)
			{
				pts = frame.frame->pkt_dts;
			}
			if (pts >= timestamp)
			{
				resultFrames = frames;
				break;
			}
			av_frame_free(&frame.frame);
			frames.pop_front();
		}
	}
	return { ret, resultFrames };
}

FCDecodeResult FCDemuxer::decodeNextPacket(const QVector<int>& streamFilter)
{
	int ret = 0;
	QList<FCFrame> frames;
	while (ret >= 0)
	{
		FCPacket packet = { 0 };
		ret = av_read_frame(_formatContext, &packet);
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
		if (!streamFilter.isEmpty() && !streamFilter.contains(packet.stream_index))
		{
			continue;
		}
		auto [err, f] = decodePacket(packet.stream_index, &packet);
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

double FCDemuxer::tsToSec(int streamIndex, int64_t timestamp) const
{
	auto tb = stream(streamIndex)->time_base;
	return timestamp * av_q2d(tb);
}

int64_t FCDemuxer::secToTs(int streamIndex, double seconds) const
{
	auto tb = stream(streamIndex)->time_base;
	return seconds * tb.den / tb.num;
}

void FCDemuxer::close()
{
	if (_formatContext)
	{
		avformat_free_context(_formatContext);
		_formatContext = nullptr;
		_streams.clear();
	}
	_decoders.clear();
}

QPair<int, QSharedPointer<FCDecoder>> FCDemuxer::getCodecContext(int streamIndex)
{
	if (auto iter = _decoders.constFind(streamIndex); iter != _decoders.cend())
	{
		return { 0, iter.value() };
	}
	int ret = 0;
	if (auto iter = _streams.constFind(streamIndex); iter != _streams.cend())
	{
		QSharedPointer<FCDecoder> decoder(new FCDecoder());
		ret = decoder->open(iter.value());
		if (ret >= 0)
		{
			_decoders.insert(streamIndex, decoder);
			return { ret, decoder };
		}
	}
	return { ret, nullptr };
}

FCDecodeResult FCDemuxer::decodePacket(int streamIndex, AVPacket *packet)
{
	auto [err, decoder] = getCodecContext(streamIndex);
	if (err < 0)
	{
		return { err, QList<FCFrame>() };
	}
	return decoder->decodePacket(packet);
}
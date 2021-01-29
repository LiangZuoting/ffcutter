#include "fcencoder.h"
#include "fcutil.h"

FCEncoder::~FCEncoder()
{
	destroy();
}

FCEncodeResult FCEncoder::encode(AVFrame *frame)
{
	FCEncodeResult result;
	if (frame)
	{
		frame->pts = _nextPts;
	}
	if (result.error = avcodec_send_frame(_context, frame); result.error == AVERROR(EAGAIN) || result.error == AVERROR_EOF)
	{
		result.error = 0;
	}
	else if (result.error < 0)
	{
		FCUtil::printAVError(result.error, "avcodec_send_frame");
	}
	else while (result.error >= 0)
	{
		AVPacket *packet = av_packet_alloc();
		if (result.error = avcodec_receive_packet(_context, packet); result.error == AVERROR(EAGAIN) || result.error == AVERROR_EOF)
		{
			result.error = 0;
			break;
		}
		if (!result.error)
		{
			av_packet_rescale_ts(packet, _context->time_base, _stream->time_base);
			packet->stream_index = _stream->index;
			result.packets.push_back(packet);
		}
		else
		{
			FCUtil::printAVError(result.error, "avcodec_receive_packet");
			break;
		}
	}
	return result;
}

FCEncodeResult FCEncoder::encode(const QList<AVFrame *> &frames)
{
	FCEncodeResult result;
	for (auto &frame : frames)
	{
		auto [err, packets] = encode(frame);
		result.error = err;
		if (err < 0)
		{
			break;
		}
		result.packets.append(packets);
	}
	return result;
}

int FCEncoder::frameSize() const
{
	if (_context && !(_context->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
	{
		return _context->frame_size;
	}
	return 0;
}

void FCEncoder::destroy()
{
	if (_context)
	{
		avcodec_free_context(&_context);
	}
	_formatContext = nullptr;
	_stream = nullptr;
}
#include "fcdecoder.h"
#include "fcutil.h"

FCDecoder::~FCDecoder()
{
	close();
}

int FCDecoder::open(const AVStream *stream)
{
	int ret = 0;
	do 
	{
		auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
		if (_context = avcodec_alloc_context3(codec); !_context)
		{
			ret = AVERROR_DECODER_NOT_FOUND;
			FCUtil::printAVError(ret, "avcodec_alloc_context3");
			break;
		}
		if ((ret = avcodec_parameters_to_context(_context, stream->codecpar)) < 0)
		{
			FCUtil::printAVError(ret, "avcodec_parameters_to_context");
			break;
		}
		if ((ret = avcodec_open2(_context, codec, nullptr)) < 0)
		{
			FCUtil::printAVError(ret, "avcodec_open2");
			break;
		}
	} while (0);
	_streamIndex = stream->index;
	return ret;
}

void FCDecoder::close()
{
	if (_context)
	{
		avcodec_free_context(&_context);
	}
}

void FCDecoder::flushBuffers()
{
	if (_context)
	{
		avcodec_flush_buffers(_context);
	}
}

FCDecodeResult FCDecoder::decodePacket(AVPacket *packet)
{
	int ret = 0;
	QList<FCFrame> frames;
	do 
	{
		if (ret = avcodec_send_packet(_context, packet); ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			ret = 0;
			break;
		}
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avcodec_send_packet");
			break;
		}
		while (ret >= 0)
		{
			AVFrame *frame = av_frame_alloc();
			if (ret = avcodec_receive_frame(_context, frame); ret >= 0)
			{
				if (frame->pts == AV_NOPTS_VALUE)
				{
					frame->pts = frame->pkt_dts;
				}
				frames.push_back({ _streamIndex, frame });
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
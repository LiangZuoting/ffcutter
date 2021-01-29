#include "fcaudioencoder.h"
#include "fcutil.h"

FCAudioEncoder::~FCAudioEncoder()
{
	destroy();
}

int FCAudioEncoder::create(AVFormatContext *formatContext, const FCMuxEntry &muxEntry)
{
	_formatContext = formatContext;
	int ret = 0;
	if (muxEntry.aStreamIndex < 0 || _formatContext->oformat->audio_codec == AV_CODEC_ID_NONE)
	{
		return ret;
	}
	do
	{
		AVCodec *codec = avcodec_find_encoder(_formatContext->oformat->audio_codec);
		_context = avcodec_alloc_context3(codec);
		_context->time_base = { 1, muxEntry.sampleRate };
		_context->bit_rate = muxEntry.aBitrate;
		_context->sample_fmt = codec->sample_fmts[0];
		for (int i = 0; codec->sample_fmts; ++i)
		{
			if (auto fmt = codec->sample_fmts[i]; fmt == AV_SAMPLE_FMT_NONE)
			{
				break;
			}
			else if (fmt == muxEntry.sampleFormat)
			{
				_context->sample_fmt = fmt;
				break;
			}
		}
		_context->sample_rate = muxEntry.sampleRate;
		for (int i = 0; codec->channel_layouts; ++i)
		{
			if (auto layout = codec->channel_layouts[i]; layout == -1)
			{
				break;
			}
			else if (layout == muxEntry.channel_layout)
			{
				_context->channel_layout = layout;
				break;
			}
		}
		if (!_context->channel_layout && codec->channel_layouts)
		{
			_context->channel_layout = codec->channel_layouts[0];
		}
		if (!_context->channel_layout)
		{
			_context->channel_layout = AV_CH_LAYOUT_STEREO;
		}
		_context->channels = av_get_channel_layout_nb_channels(_context->channel_layout);
		if (_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
		{
			_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}

		_stream = avformat_new_stream(_formatContext, codec);
		_stream->id = _formatContext->nb_streams - 1;
		_stream->time_base = { 1, _context->sample_rate };
		if (ret = avcodec_parameters_from_context(_stream->codecpar, _context); ret < 0)
		{
			FCUtil::printAVError(ret, "avcodec_parameters_from_context");
			break;
		}
		if (ret = avcodec_open2(_context, codec, nullptr); ret < 0)
		{
			FCUtil::printAVError(ret, "avcodec_open2");
			break;
		}
	} while (0);
	return ret;
}

FCEncodeResult FCAudioEncoder::encode(AVFrame *frame)
{
	auto result = FCEncoder::encode(frame);
	if (frame)
	{
		_nextPts += frame->nb_samples;
	}
	return result;
}
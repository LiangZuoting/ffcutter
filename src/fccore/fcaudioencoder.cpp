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
		auto codec = avcodec_find_encoder(_formatContext->oformat->audio_codec);
		_context = avcodec_alloc_context3(codec);
		const AVSampleFormat* sampleFormats{};
		int numOfConfigs{};
		if (ret = avcodec_get_supported_config(_context, codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, reinterpret_cast<const void**>(&sampleFormats), &numOfConfigs); ret)
		{
			FCUtil::printAVError(ret, "avcodec_get_supported_config");
			break;
		}
		assert(numOfConfigs > 0);
		if (numOfConfigs > 0)
		{
			_context->sample_fmt = sampleFormats[0];
		}
		_context->sample_rate = 44100;
		av_channel_layout_default(&_context->ch_layout, 2);
		_context->time_base = { 1, _context->sample_rate };
		if (_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
		{
			_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}

		_stream = avformat_new_stream(_formatContext, codec);
		_stream->id = _formatContext->nb_streams - 1;
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
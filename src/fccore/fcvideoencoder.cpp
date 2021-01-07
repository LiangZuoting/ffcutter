#include "fcvideoencoder.h"
#include "fcutil.h"
extern "C"
{
#include <libavutil/opt.h>
}

FCVideoEncoder::~FCVideoEncoder()
{
	destroy();
}

int FCVideoEncoder::create(AVFormatContext *formatContext, const FCMuxEntry &muxEntry)
{
	_formatContext = formatContext;
	int ret = 0;
	do
	{
		if (auto codecId = _formatContext->oformat->video_codec; codecId != AV_CODEC_ID_NONE)
		{
			auto codec = avcodec_find_encoder(codecId);
			_context = avcodec_alloc_context3(codec);
			_context->time_base = { 1, muxEntry.fps };
			_context->bit_rate = muxEntry.vBitrate;
			_context->width = muxEntry.width;
			_context->height = muxEntry.height;
			_context->gop_size = muxEntry.gop;
			// 请求的格式优先，不支持时用第一个
			_context->pix_fmt = codec->pix_fmts[0];
			for (int i = 0; ; ++i)
			{
				auto fmt = codec->pix_fmts[i];
				if (fmt == AV_PIX_FMT_NONE)
				{
					break;
				}
				if (fmt == muxEntry.pixelFormat)
				{
					_context->pix_fmt = fmt;
					break;
				}
			}
			_context->framerate = { 1, muxEntry.fps };
			/*
			* H264 codec 不能 set 这个 flag，否则文件不能解析；
			* gif 必须 set 这个 flag，否则图像效果不对。
			*/
			if (_formatContext->oformat->flags & AVFMT_GLOBALHEADER && _context->codec_id != AV_CODEC_ID_H264)
			{
				_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}

			_stream = avformat_new_stream(_formatContext, codec);
			_stream->id = _formatContext->nb_streams - 1;
			_stream->time_base = _context->time_base;
			_stream->avg_frame_rate = _context->framerate;
			int ret = avcodec_parameters_from_context(_stream->codecpar, _context);
			if (ret)
			{
				FCUtil::printAVError(ret, "avcodec_parameters_from_context");
				break;
			}

			ret = av_opt_set_int(_context, "crf", 23, AV_OPT_SEARCH_CHILDREN);
			if (ret)
			{
				FCUtil::printAVError(ret, "av_opt_set_int");
			}

			ret = avcodec_open2(_context, codec, nullptr);
			if (ret)
			{
				FCUtil::printAVError(ret, "avcodec_open2");
				break;
			}
		}
	} while (0);
	return ret;
}

FCEncodeResult FCVideoEncoder::encode(AVFrame *frame)
{
	auto result = FCEncoder::encode(frame);
	++_nextPts;
	return result;
}
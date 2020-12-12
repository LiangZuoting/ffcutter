#include "fcmuxer.h"
#include "fcutil.h"

FCMuxer::~FCMuxer()
{
	destroy();
}

int FCMuxer::create(const FCMuxEntry &muxEntry)
{
	int ret = 0;
	auto codecId = formatContext->oformat->video_codec;
	if (codecId != AV_CODEC_ID_NONE)
	{
		auto videoCodec = avcodec_find_encoder(codecId);
		_videoCodec = avcodec_alloc_context3(videoCodec);
		_videoCodec->time_base = { 1, muxEntry.fps };
		_videoCodec->width = muxEntry.width;
		_videoCodec->height = muxEntry.height;
		if (auto fmt = videoCodec->pix_fmts[0]; fmt != AV_PIX_FMT_NONE)
		{
			_videoCodec->pix_fmt = fmt;
		}
		_videoCodec->framerate = { 1, muxEntry.fps };
		if (formatContext->oformat->flags & AVFMT_GLOBALHEADER && _videoCodec->codec_id != AV_CODEC_ID_H264)
		{
			_videoCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
		
		auto videoStream = avformat_new_stream(formatContext, videoCodec);
		videoStream->id = formatContext->nb_streams - 1;
		videoStream->time_base = _videoCodec->time_base;
		videoStream->avg_frame_rate = _videoCodec->framerate;
		do 
		{
			ret = avcodec_parameters_from_context(videoStream->codecpar, _videoCodec);
			if (ret)
			{
				FCUtil::printAVError(ret, "avcodec_parameters_from_context");
				break;
			}
			ret = avcodec_open2(_videoCodec, videoCodec, nullptr);
			if (ret)
			{
				FCUtil::printAVError(ret, "avcodec_open2");
				break;
			}
		} while (0);
	}

	return ret;
}

QPair<int, QList<AVFrame *>> FCMuxer::encodeVideo(const AVFrame *frame)
{
	QPair<int, QList<AVFrame *>> result;
	
	result.first = avcodec_send_frame(_videoCodec, frame);
	if (result.first == AVERROR(EAGAIN) || result.first == AVERROR_EOF)
	{
		result.first = 0;
	}
	else while (true)
	{
		result.first = avcodec_receive_packet(_videoCodec,)
	}
}
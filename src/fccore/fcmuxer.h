#pragma once

#include "fcmuxentry.h"
#include <QPair>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class FCMuxer
{
public:
	~FCMuxer();

	int create(const FCMuxEntry &muxEntry);

	QPair<int, QList<AVFrame *>> encodeVideo(const AVFrame *frame);

	void destroy();

private:
	AVFormatContext *_formatContext = nullptr;
	AVCodecContext *_audioCodec = nullptr;
	AVCodecContext *_videoCodec = nullptr;
	AVCodecContext *_subtitleCodec = nullptr;
};
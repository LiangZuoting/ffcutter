#pragma once

#include "fcconst.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class FCEncoder
{
public:
	virtual ~FCEncoder();

	virtual int create(AVFormatContext *formatContext, const FCMuxEntry &muxEntry) = 0;
	virtual FCEncodeResult encode(AVFrame *frame);
	virtual FCEncodeResult encode(const QList<AVFrame *> &frames);
	virtual int frameSize() const;
	virtual void destroy();

	AVStream *stream() const
	{
		return _stream;
	}

	virtual int format() const = 0;

protected:
	AVCodecContext *_context = nullptr;
	AVFormatContext *_formatContext = nullptr;
	AVStream *_stream = nullptr;
	int64_t _nextPts = 0;
};
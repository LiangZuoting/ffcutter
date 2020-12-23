#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include "fcconst.h"

class FCDecoder
{
public:
	~FCDecoder();

	int open(const AVStream *stream);
	void close();

	void flushBuffers();
	FCDecodeResult decodePacket(AVPacket *packet);

private:
	AVCodecContext *_context = nullptr;
	int _streamIndex = -1;
};
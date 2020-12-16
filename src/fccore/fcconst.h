#pragma once

#include <QList>
extern "C"
{
#include <libavutil/frame.h>
}

struct FCFrame
{
	int streamIndex = -1;
	AVFrame *frame = nullptr;
};

struct FCDecodeResult
{
	int error = 0;
	QList<FCFrame> frames;
};

struct FCFilterResult
{
	int error = 0;
	QList<AVFrame *> frames;
};
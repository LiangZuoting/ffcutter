#pragma once

#include <QList>
extern "C"
{
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
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

struct FCEncodeResult
{
	int error = 0;
	QList<AVPacket *> packets;
};

/// <summary>
/// 析构时自动调用 av_packet_unref()
/// </summary>
struct FCPacket : AVPacket
{
	~FCPacket()
	{
		av_packet_unref(this);
	}
};
#pragma once

#include <QList>
extern "C"
{
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
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

struct FCFilterParametersBase
{
	int64_t startPts = AV_NOPTS_VALUE;
	int64_t endPts = AV_NOPTS_VALUE;
	QString filterString;
};

struct FCMuxEntry
{
	QString filePath;
	double startSec = 0; // start time in second
	double endSec = 0; // end time in second
	// video
	int vStreamIndex = -1;
	AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;
	int vBitrate = 0;
	int width = 0;
	int height = 0;
	int fps = 0;
	int gop = 12;
	QList<FCFilterParametersBase> vFilters;
	// audio
	int aStreamIndex = -1;
	AVSampleFormat sampleFormat = AV_SAMPLE_FMT_NONE;
	int aBitrate = 0;
	int sampleRate = 0;
	int channels = 0;
	uint64_t channel_layout = 0;
	QString aFilterString;
};
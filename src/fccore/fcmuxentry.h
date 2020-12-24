#pragma once

#include "fccore_global.h"
#include <QString>
extern "C"
{
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}

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
	int gop =  12;
	QString vFilterString;
	// audio
	int aStreamIndex = -1;
	AVSampleFormat sampleFormat = AV_SAMPLE_FMT_NONE;
	int aBitrate = 0;
	int sampleRate = 0;
	int channels = 0;
	uint64_t channel_layout = 0;
	QString aFilterString;
};


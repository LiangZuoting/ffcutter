#pragma once

#include "fccore_global.h"
#include <QString>
extern "C"
{
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
}

struct FCMuxEntry
{
	QString filePath;
	double startSec = 0; // start time in second
	double endSec = 0; // end time in second
	// video
	int vStreamIndex = -1;
	int width = 0;
	int height = 0;
	int fps = 0;
	int gop =  5; // in seconds
	QString vFilterString;
	// audio
	int aStreamIndex = -1;
	QString aFilterString;
};


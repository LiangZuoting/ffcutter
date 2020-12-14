#pragma once

#include "fccore_global.h"
#include <QString>

enum FCDurationUnit
{
	DURATION_SECOND,
	DURATION_FRAME_COUNT,
};

struct FCMuxEntry
{
	QString filePath;
	int64_t startPts = 0; // same with input stream's time_base
	double duration = 0;
	FCDurationUnit durationUnit = DURATION_SECOND;
	// video
	int vStreamIndex = -1;
	int width = 0;
	int height = 0;
	int fps = 0;
	QString filterString;
	// audio
};


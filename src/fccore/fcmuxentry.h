#pragma once

#include "fccore_global.h"
#include <QString>

struct FCMuxEntry
{
	QString filePath;
	double startSec = 0; // start time in second
	double durationSec = 0; // duration in second
	// video
	int vStreamIndex = -1;
	int width = 0;
	int height = 0;
	int fps = 0;
	QString filterString;
	// audio
	int aStreamIndex = -1;
};


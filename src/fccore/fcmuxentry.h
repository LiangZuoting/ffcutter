#pragma once

#include "fccore_global.h"

enum FCDurationUnit
{
	DURATION_FRAME_COUNT,
	DURATION_SECOND,
};

struct FCMuxEntry
{
	FCMuxEntry() = default;

	FCMuxEntry(const FCMuxEntry& rhs)
	{
		memcpy(this, &rhs, sizeof(FCMuxEntry));
		auto len = strlen(rhs.filePath) + 1;
		filePath = new char[len] {0};
		memcpy(filePath, rhs.filePath, len);
	}

	FCMuxEntry &operator = (const FCMuxEntry &rhs)
	{
		if (&rhs != this)
		{
			memcpy(this, &rhs, sizeof(FCMuxEntry));
			auto len = strlen(rhs.filePath) + 1;
			filePath = new char[len] {0};
			memcpy(filePath, rhs.filePath, len);
		}
	}

	~FCMuxEntry()
	{
// 		if (filePath)
// 		{
// 			delete[] filePath;
// 			filePath = nullptr;
// 		}
	}

	char *filePath = nullptr;
	int64_t startPts = 0; // same with input stream's time_base
	double duration = 0;
	FCDurationUnit durationUnit = DURATION_FRAME_COUNT;
	// video
	int vStreamIndex = -1;
	int width = 0;
	int height = 0;
	int fps = 0;
	// audio
};


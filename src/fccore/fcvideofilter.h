#pragma once
#include "fcfilter.h"

struct FCVideoFilterParameters : FCFilterParameters
{
	int srcWidth = 0;
	int srcHeight = 0;
	AVPixelFormat srcPixelFormat = AV_PIX_FMT_NONE;
	AVRational srcSampleAspectRatio = { 0 };
	AVPixelFormat dstPixelFormat = AV_PIX_FMT_NONE;
};

class FCVideoFilter : public FCFilter
{
public:
	int create(const FCFilterParameters& params) override;

protected:
	int setSinkFilter(const FCFilterParameters &params) override;
};
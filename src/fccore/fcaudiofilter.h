#pragma once
#include "fcfilter.h"

struct FCAudioFilterParameters : FCFilterParameters
{
	AVSampleFormat srcSampleFormat = AV_SAMPLE_FMT_NONE;
	int srcSampleRate = 0;
	AVChannelLayout* srcChannelLayout{nullptr};
	AVSampleFormat dstSampleFormat = AV_SAMPLE_FMT_NONE;
	int dstSampleRate = 0;
	AVChannelLayout* dstChannelLayout{nullptr};
	int frameSize = 0;
};

class FCAudioFilter : public FCFilter
{
public:
	int create(const FCFilterParameters& params) override;

	AVMediaType type() const override
	{
		return AVMEDIA_TYPE_AUDIO;
	}

protected:
	int setSinkFilter(const FCFilterParameters &params) override;
};
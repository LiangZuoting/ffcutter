#pragma once

#include "fcencoder.h"

class FCAudioEncoder : public FCEncoder
{
public:
	~FCAudioEncoder() override;

	int create(AVFormatContext *formatContext, const FCMuxEntry &muxEntry) override;
	FCEncodeResult encode(AVFrame *frame) override;
	int format() const override
	{
		if (_context)
		{
			return _context->sample_fmt;
		}
		return AV_SAMPLE_FMT_NONE;
	}
};
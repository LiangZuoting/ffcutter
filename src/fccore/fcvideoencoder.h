#pragma once

#include "fcencoder.h"

class FCVideoEncoder : public FCEncoder
{
public:
	~FCVideoEncoder() override;

	int create(AVFormatContext *formatContext, const FCMuxEntry &muxEntry) override;
	FCEncodeResult encode(AVFrame *frame) override;
	int format() const override
	{
		if (_context)
		{
			return _context->pix_fmt;
		}
		return AV_PIX_FMT_NONE;
	}
};
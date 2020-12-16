#pragma once
#include <QPair>

extern "C"
{
#include <libavfilter/avfilter.h>
}
#include "fcconst.h"

class FCFilter
{
public:
	~FCFilter();

	int create(int srcWidth, int srcHeight, AVPixelFormat srcPixelFormat, const AVRational& srcTimeBase, const AVRational& srcSampleAspectRatio, const char *filters, AVPixelFormat dstPixelFormat);
	FCFilterResult filter(AVFrame* frame);
	void destroy();

private:
	FCFilterResult flush();

	AVFilterGraph *_graph = nullptr;
	AVFilterContext *_srcContext = nullptr;
	AVFilterContext *_sinkContext = nullptr;
	AVFilterInOut *_inputs = nullptr;
	AVFilterInOut *_outputs = nullptr;
};
#pragma once
#include <QPair>

extern "C"
{
#include <libavfilter/avfilter.h>
}

class FCFilter
{
public:
	~FCFilter();

	int create(const char* filters, int width, int height, AVPixelFormat pixelFormat, const AVRational& timeBase, const AVRational& sampleAspectRatio);
	QPair<int, QList<AVFrame*>> filter(AVFrame* frame);
	void destroy();

private:
	AVFilterGraph *_graph = nullptr;
	AVFilterContext *_srcContext = nullptr;
	AVFilterContext *_sinkContext = nullptr;
	AVFilterInOut *_inputs = nullptr;
	AVFilterInOut *_outputs = nullptr;
};
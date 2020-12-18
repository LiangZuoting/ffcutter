#pragma once
#include <QPair>

extern "C"
{
#include <libavfilter/avfilter.h>
}
#include "fcconst.h"

struct FCFilterParameters
{
	AVRational srcTimeBase = { 0 };
	QString filterString;
};

class FCFilter
{
public:
	virtual ~FCFilter();

	virtual int create(const FCFilterParameters &params) = 0;
	virtual FCFilterResult filter(AVFrame* frame);
	virtual void destroy();

protected:
	virtual int create(const FCFilterParameters &params, const AVFilter *srcFilter, const char *args, const AVFilter *sinkFilter);
	virtual int setSinkFilter(const FCFilterParameters &params) = 0;
	virtual FCFilterResult flush();

	AVFilterGraph *_graph = nullptr;
	AVFilterContext *_srcContext = nullptr;
	AVFilterContext *_sinkContext = nullptr;
	AVFilterInOut *_inputs = nullptr;
	AVFilterInOut *_outputs = nullptr;
};
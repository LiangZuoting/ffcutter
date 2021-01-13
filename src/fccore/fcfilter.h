#pragma once
#include <QPair>

extern "C"
{
#include <libavfilter/avfilter.h>
}
#include "fcconst.h"

struct FCFilterParameters : FCFilterParametersBase
{
	AVRational srcTimeBase = { 0 };
};

class FCFilter
{
public:
	virtual ~FCFilter();

	virtual int create(const FCFilterParameters &params);
	virtual FCFilterResult filter(AVFrame* frame);
	virtual void destroy();
	virtual int checkPtsRange(const AVFrame *frame);
	virtual AVMediaType type() const = 0;

protected:
	virtual int create(const FCFilterParameters &params, const AVFilter *srcFilter, const char *args, const AVFilter *sinkFilter);
	virtual int setSinkFilter(const FCFilterParameters &params) = 0;
	virtual FCFilterResult flush();

	int64_t _startPts = AV_NOPTS_VALUE;
	int64_t _endPts = AV_NOPTS_VALUE;
	AVFilterGraph *_graph = nullptr;
	AVFilterContext *_srcContext = nullptr;
	AVFilterContext *_sinkContext = nullptr;
	AVFilterInOut *_inputs = nullptr;
	AVFilterInOut *_outputs = nullptr;
};
#include "fcfilter.h"
#include "fcutil.h"
extern "C"
{
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
}

FCFilter::~FCFilter()
{
	destroy();
}

FCFilterResult FCFilter::filter(AVFrame *frame)
{
	if (!frame)
	{
		return flush();
	}

	int ret = 0;
	QList<AVFrame*> frames;
	while (ret >= 0)
	{
		if (ret = av_buffersrc_add_frame_flags(_srcContext, frame, AV_BUFFERSRC_FLAG_KEEP_REF); ret < 0)
		{
			FCUtil::printAVError(ret, "av_buffersrc_add_frame_flags");
			break;
		}
		while (ret >= 0)
		{
			AVFrame *dstFrame = av_frame_alloc();
			if (ret = av_buffersink_get_frame(_sinkContext, dstFrame); ret >= 0)
			{
				frames.push_back(dstFrame);
				continue;
			}
			av_frame_free(&dstFrame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				break;
			}
			FCUtil::printAVError(ret, "av_buffersink_get_frame");
			break;
		}
	}
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		ret = 0;
	}
	return { ret, frames };
}

FCFilterResult FCFilter::flush()
{
	int ret = 0;
	QList<AVFrame*> frames;
	while (ret >= 0)
	{
		AVFrame *dstFrame = av_frame_alloc();
		if (ret = av_buffersink_get_frame_flags(_sinkContext, dstFrame, AV_BUFFERSINK_FLAG_NO_REQUEST); ret >= 0)
		{
			frames.push_back(dstFrame);
			continue;
		}
		av_frame_free(&dstFrame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			ret = 0;
			break;
		}
		FCUtil::printAVError(ret, "av_buffersink_get_frame_flags");
		break;
	}
	return { ret, frames };
}

void FCFilter::destroy()
{
	avfilter_inout_free(&_inputs);
	avfilter_inout_free(&_outputs);
	if (_srcContext)
	{
		avfilter_free(_srcContext);
		_srcContext = nullptr;
	}
	if (_sinkContext)
	{
		avfilter_free(_sinkContext);
		_sinkContext = nullptr;
	}
	avfilter_graph_free(&_graph);
}

int FCFilter::create(const FCFilterParameters &params, const AVFilter *srcFilter, const char *args, const AVFilter *sinkFilter)
{
	_inputs = avfilter_inout_alloc();
	_outputs = avfilter_inout_alloc();
	_graph = avfilter_graph_alloc();
	if (!_inputs || !_outputs || !_graph)
	{
		return AVERROR(ENOMEM);
	}
	int ret = 0;
	do
	{
		if (ret = avfilter_graph_create_filter(&_srcContext, srcFilter, "in", args, nullptr, _graph); ret < 0)
		{
			FCUtil::printAVError(ret, "avfilter_graph_create_filter");
			break;
		}
		if (ret = avfilter_graph_create_filter(&_sinkContext, sinkFilter, "out", nullptr, nullptr, _graph); ret < 0)
		{
			FCUtil::printAVError(ret, "avfilter_graph_create_filter");
			break;
		}
		if (ret = setSinkFilter(params); ret < 0)
		{
			break;
		}
		_inputs->name = av_strdup("out");
		_inputs->filter_ctx = _sinkContext;
		_inputs->pad_idx = 0;
		_inputs->next = nullptr;
		_outputs->name = av_strdup("in");
		_outputs->filter_ctx = _srcContext;
		_outputs->pad_idx = 0;
		_outputs->next = nullptr;
		auto strFilter = params.filterString.toStdString();
		if (ret = avfilter_graph_parse_ptr(_graph, strFilter.data(), &_inputs, &_outputs, nullptr); ret < 0)
		{
			FCUtil::printAVError(ret, "avfilter_graph_parse_ptr");
			break;
		}
		if (ret = avfilter_graph_config(_graph, nullptr); ret < 0)
		{
			FCUtil::printAVError(ret, "avfilter_graph_config");
			break;
		}
	} while (0);
	return ret;
}
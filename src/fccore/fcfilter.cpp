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

int FCFilter::create(int srcWidth, int srcHeight, AVPixelFormat srcPixelFormat, const AVRational &srcTimeBase, const AVRational &srcSampleAspectRatio, const char *filters, AVPixelFormat dstPixelFormat)
{
	const AVFilter *srcFilter = avfilter_get_by_name("buffer");
	const AVFilter *sinkFilter = avfilter_get_by_name("buffersink");
	_inputs = avfilter_inout_alloc();
	_outputs = avfilter_inout_alloc();
	_graph = avfilter_graph_alloc();
	if (!_inputs || !_outputs || !_graph)
	{
		return AVERROR(ENOMEM);
	}
	auto args = QString("width=%1:height=%2:pix_fmt=%3:time_base=%4/%5:pixel_aspect=%6/%7")
			.arg(srcWidth).arg(srcHeight).arg(srcPixelFormat).arg(srcTimeBase.num)
			.arg(srcTimeBase.den).arg(srcSampleAspectRatio.num).arg(srcSampleAspectRatio.den).toStdString();
	int ret = 0;
	do 
	{
		ret = avfilter_graph_create_filter(&_srcContext, srcFilter, "in", args.data(), nullptr, _graph);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avfilter_graph_create_filter");
			break;
		}
		ret = avfilter_graph_create_filter(&_sinkContext, sinkFilter, "out", nullptr, nullptr, _graph);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avfilter_graph_create_filter");
			break;
		}
		AVPixelFormat pix_fmts[] = { dstPixelFormat, AV_PIX_FMT_NONE };
		ret = av_opt_set_int_list(_sinkContext, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "av_opt_set_int_list");
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
		ret = avfilter_graph_parse_ptr(_graph, filters, &_inputs, &_outputs, nullptr);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avfilter_graph_parse_ptr");
			break;
		}
		ret = avfilter_graph_config(_graph, nullptr);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avfilter_graph_config");
			break;
		}
	} while (0);
#if DEBUG
	if (_graph)
	{
		FCUtil::printAVFilterGraph("d:\\fcfilter.txt", _graph);
	}
#endif
	return ret;
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
			ret = av_buffersink_get_frame(_sinkContext, dstFrame);
			if (ret >= 0)
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
		ret = av_buffersink_get_frame_flags(_sinkContext, dstFrame, AV_BUFFERSINK_FLAG_NO_REQUEST);
		if (ret >= 0)
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
#include "fcaudiofilter.h"
#include "fcutil.h"
extern "C"
{
#include <libavutil/opt.h>
#include <libavfilter/buffersink.h>
}

int FCAudioFilter::create(const FCFilterParameters& params)
{
	const AVFilter *srcFilter = avfilter_get_by_name("abuffer");
	const AVFilter *sinkFilter = avfilter_get_by_name("abuffersink");
	auto aParams = static_cast<const FCAudioFilterParameters *>(&params);
	auto args = QString("time_base=%1/%2:sample_rate=%3:sample_fmt=%4:channel_layout=0x%5")
		.arg(aParams->srcTimeBase.num).arg(aParams->srcTimeBase.den)
		.arg(aParams->srcSampleRate).arg(av_get_sample_fmt_name(aParams->srcSampleFormat))
		.arg(aParams->srcChannelLayout, 0, 16).toStdString();
	int ret = FCFilter::create(params, srcFilter, args.data(), sinkFilter);
#if DEBUG
	if (ret >= 0)
	{
		FCUtil::printAVFilterGraph("d:\\fcaudiofilter.txt", _graph);
	}
#endif
	if (aParams->frameSize > 0) // ÒôÆµ±àÂëÆ÷ÒªÇó¹Ì¶¨³ß´çµÄÊäÈë
	{
		av_buffersink_set_frame_size(_sinkContext, aParams->frameSize);
	}
	return ret;
}

int FCAudioFilter::setSinkFilter(const FCFilterParameters &params)
{
	auto aParams = static_cast<const FCAudioFilterParameters *>(&params);
	AVSampleFormat fmts[] = { aParams->dstSampleFormat, AV_SAMPLE_FMT_NONE };
	int ret = av_opt_set_int_list(_sinkContext, "sample_fmts", fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0)
	{
		FCUtil::printAVError(ret, "av_opt_set_int_list", "sample_fmts");
		return ret;
	}
	int64_t layouts[] = { aParams->dstChannelLayout, -1 };
	ret = av_opt_set_int_list(_sinkContext, "channel_layouts", layouts, -1, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0)
	{
		FCUtil::printAVError(ret, "av_opt_set_int_list", "channel_layouts");
		return ret;
	}
	int rates[] = { aParams->dstSampleRate, -1 };
	ret = av_opt_set_int_list(_sinkContext, "sample_rates", rates, -1, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0)
	{
		FCUtil::printAVError(ret, "av_opt_set_int_list", "sample_rates");
		return ret;
	}
	return ret;
}
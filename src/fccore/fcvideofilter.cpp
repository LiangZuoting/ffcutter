#include "fcvideofilter.h"
#include "fcutil.h"
extern "C"
{
#include <libavutil/opt.h>
}

int FCVideoFilter::create(const FCFilterParameters& params)
{
	const AVFilter *srcFilter = avfilter_get_by_name("buffer");
	const AVFilter *sinkFilter = avfilter_get_by_name("buffersink");
	auto vParams = static_cast<const FCVideoFilterParameters *>(&params);
	auto args = QString("width=%1:height=%2:pix_fmt=%3:time_base=%4/%5:pixel_aspect=%6/%7")
		.arg(vParams->srcWidth).arg(vParams->srcHeight).arg(vParams->srcPixelFormat).arg(vParams->srcTimeBase.num)
		.arg(vParams->srcTimeBase.den).arg(vParams->srcSampleAspectRatio.num).arg(vParams->srcSampleAspectRatio.den).toStdString();
	int ret = FCFilter::create(params, srcFilter, args.data(), sinkFilter);
#if DEBUG
	if (ret >= 0)
	{
		FCUtil::printAVFilterGraph("d:\\fcvideofilter.txt", _graph);
	}
#endif
	return ret;
}

int FCVideoFilter::setSinkFilter(const FCFilterParameters &params)
{
	auto vParams = static_cast<const FCVideoFilterParameters *>(&params);
	AVPixelFormat fmts[] = { vParams->dstPixelFormat, AV_PIX_FMT_NONE };
	int ret = av_opt_set_int_list(_sinkContext, "pix_fmts", fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0)
	{
		FCUtil::printAVError(ret, "av_opt_set_int_list");
	}
	return ret;
}
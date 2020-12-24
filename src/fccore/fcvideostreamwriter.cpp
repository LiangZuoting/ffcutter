#include "fcvideostreamwriter.h"
#include "fcvideofilter.h"
extern "C"
{
#include <libavutil/pixdesc.h>
}

FCVideoStreamWriter::FCVideoStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer)
	: FCStreamWriter(entry, demuxer, muxer)
{
	
}

int FCVideoStreamWriter::create()
{
	_inStreamIndex = _entry.vStreamIndex;
	_startPts = _demuxer->secToTs(_inStreamIndex, _entry.startSec);
	_endPts = _demuxer->secToTs(_inStreamIndex, _entry.endSec);
	return createFilter();
}

bool FCVideoStreamWriter::eof() const
{
	return !_muxer.videoStream() || _eof;
}

int FCVideoStreamWriter::write(const FCFrame& frame)
{
	return FCStreamWriter::write(AVMEDIA_TYPE_VIDEO, frame);
}

int FCVideoStreamWriter::createFilter()
{
	if (!_muxer.videoStream())
	{
		return 0;
	}
	_filter.reset(new FCVideoFilter());
	auto filters = _entry.vFilterString;
	if (!filters.isEmpty())
	{
		filters.append(',');
	}
	filters.append("format=").append(av_get_pix_fmt_name(_muxer.videoFormat()));
	auto srcStream = _demuxer->stream(_inStreamIndex);
	FCVideoFilterParameters params{};
	params.srcWidth = srcStream->codecpar->width;
	params.srcHeight = srcStream->codecpar->height;
	params.srcPixelFormat = (AVPixelFormat)srcStream->codecpar->format;
	params.srcSampleAspectRatio = srcStream->sample_aspect_ratio;
	params.dstPixelFormat = _muxer.videoFormat();
	params.srcTimeBase = srcStream->time_base;
	params.filterString = filters;
	return _filter->create(params);
}
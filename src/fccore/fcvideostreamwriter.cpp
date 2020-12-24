#include "fcvideostreamwriter.h"
#include "fcvideofilter.h"
extern "C"
{
#include <libavutil/pixdesc.h>
}

FCVideoStreamWriter::FCVideoStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer)
	: _entry(entry)
	, _demuxer(demuxer)
	, _muxer(muxer)
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

void FCVideoStreamWriter::setEof()
{
	_eof = true;
}

int FCVideoStreamWriter::write(const FCFrame& frame)
{
	QList<AVFrame*> frames;
	if (_inStreamIndex != frame.streamIndex || _eof)
	{
		return 0;
	}
	int ret = checkPtsRange(frame);
	if (ret < 0)
	{
		return 0;
	}
	if (ret > 0)
	{
		_eof = true;
	}
	if (_filter)
	{
		auto filterResult = _filter->filter(_eof ? nullptr : frame.frame);
		ret = filterResult.error;
		if (ret < 0)
		{
			return ret;
		}
		frames = filterResult.frames;
	}
	else if (!_eof)
	{
		frames.push_back(frame.frame);
	}
	if (_eof)
	{
		frames.push_back(nullptr);
	}
	ret = _muxer.write(AVMEDIA_TYPE_VIDEO, frames);
	if (_filter)
	{
		clearFrames(frames);
	}
	return ret;
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

int FCVideoStreamWriter::checkPtsRange(const FCFrame& frame)
{
	if (frame.frame->pts < _startPts)
	{
		return -1;
	}
	if (frame.frame->pts > _endPts)
	{
		return 1;
	}
	return 0;
}

void FCVideoStreamWriter::clearFrames(QList<AVFrame*>& frames)
{
	for (auto& f : frames)
	{
		av_frame_free(&f);
	}
	frames.clear();
}
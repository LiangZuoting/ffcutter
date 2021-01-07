#include "fcstreamwriter.h"
#include "fcvideofilter.h"
extern "C"
{
#include <libavutil/pixdesc.h>
}

FCStreamWriter::FCStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer)
	: _entry(entry)
	, _demuxer(demuxer)
	, _muxer(muxer)
{
	
}

void FCStreamWriter::setEof()
{
	_eof = true;
}

int FCStreamWriter::write(AVMediaType type, const FCFrame& frame)
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
	ret = _muxer.write(type, frames);
	if (_filter)
	{
		clearFrames(frames);
	}
	return ret;
}

int FCStreamWriter::checkPtsRange(const FCFrame& frame)
{
	if (!frame.frame) // ´«Èë¿ÕÖ¡£¬eof
	{
		return 1;
	}
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

void FCStreamWriter::clearFrames(QList<AVFrame*>& frames)
{
	for (auto& f : frames)
	{
		av_frame_free(&f);
	}
	frames.clear();
}
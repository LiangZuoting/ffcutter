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
	if (_inStreamIndex != frame.streamIndex || _eof)
	{
		return 0;
	}
	QList<AVFrame *> frames;
	if (_filters.isEmpty())
	{
		_eof = !frame.frame;
		frames.push_back(frame.frame);
	}
	int ret = 0;
	for (int i = _currentFilter; i < _filters.size(); ++i)
	{
		_currentFilter = i;
		bool eof = false;
		auto filter = _filters[i];
		ret = filter->checkPtsRange(frame.frame);
		if (ret < 0)
		{
			return 0;
		}
		if (ret > 0)
		{
			if (i == _filters.size() - 1)
			{
				_eof = true;
			}
			eof = true;
		}
		auto filterResult = filter->filter(eof ? nullptr : frame.frame);
		ret = filterResult.error;
		if (ret < 0)
		{
			return ret;
		}
		frames = filterResult.frames;
	}
	if (_eof)
	{
		frames.push_back(nullptr);
	}
	ret = _muxer.write(type, frames);
	if (!_filters.isEmpty())
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
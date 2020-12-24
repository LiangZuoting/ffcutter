#include "fcaudiostreamwriter.h"
#include "fcaudiofilter.h"
extern "C"
{
#include <libavutil/pixdesc.h>
}

FCAudioStreamWriter::FCAudioStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer)
	: _entry(entry)
	, _demuxer(demuxer)
	, _muxer(muxer)
{
	
}

int FCAudioStreamWriter::create()
{
	_inStreamIndex = _entry.aStreamIndex;
	_startPts = _demuxer->secToTs(_inStreamIndex, _entry.startSec);
	_endPts = _demuxer->secToTs(_inStreamIndex, _entry.endSec);
	return createFilter();
}

bool FCAudioStreamWriter::eof() const
{
	return !_muxer.videoStream() || _eof;
}

void FCAudioStreamWriter::setEof()
{
	_eof = true;
}

int FCAudioStreamWriter::write(const FCFrame& frame)
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
	ret = _muxer.write(AVMEDIA_TYPE_AUDIO, frames);
	if (_filter)
	{
		clearFrames(frames);
	}
	return ret;
}

int FCAudioStreamWriter::createFilter()
{
	auto dstStream = _muxer.audioStream();
	if (!dstStream)
	{
		return 0;
	}
	_filter.reset(new FCAudioFilter());
	auto filters = _entry.aFilterString;
	if (!filters.isEmpty())
	{
		filters.append(',');
	}
	char layout[100] = { 0 };
	av_get_channel_layout_string(layout, 100, dstStream->codecpar->channels, dstStream->codecpar->channel_layout);
	filters.append(QString("aresample=%3,aformat=sample_fmts=%1:channel_layouts=%2")
		.arg(av_get_sample_fmt_name((AVSampleFormat)dstStream->codecpar->format))
		.arg(layout).arg(dstStream->codecpar->sample_rate));
	auto srcStream = _demuxer->stream(_inStreamIndex);
	FCAudioFilterParameters params{};
	params.srcTimeBase = srcStream->time_base;
	params.srcSampleFormat = (AVSampleFormat)srcStream->codecpar->format;
	params.srcSampleRate = srcStream->codecpar->sample_rate;
	params.srcChannelLayout = srcStream->codecpar->channel_layout;
	params.dstSampleFormat = (AVSampleFormat)dstStream->codecpar->format;
	params.dstSampleRate = dstStream->codecpar->sample_rate;
	params.dstChannelLayout = dstStream->codecpar->channel_layout;
	params.filterString = filters;
	params.frameSize = _muxer.fixedAudioFrameSize();
	return _filter->create(params);
}

int FCAudioStreamWriter::checkPtsRange(const FCFrame& frame)
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

void FCAudioStreamWriter::clearFrames(QList<AVFrame*>& frames)
{
	for (auto& f : frames)
	{
		av_frame_free(&f);
	}
	frames.clear();
}
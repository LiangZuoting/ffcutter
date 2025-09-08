#include "fcaudiostreamwriter.h"
#include "fcaudiofilter.h"

FCAudioStreamWriter::FCAudioStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer)
	: FCStreamWriter(entry, demuxer, muxer)
{
	
}

int FCAudioStreamWriter::create()
{
	_inStreamIndex = _entry.aStreamIndex;
	if (_inStreamIndex < 0)
	{
		return 0;
	}
	_startPts = _demuxer->secToTs(_inStreamIndex, _entry.startSec);
	_endPts = _demuxer->secToTs(_inStreamIndex, _entry.endSec);
	return createFilter();
}

bool FCAudioStreamWriter::eof() const
{
	return !_muxer.audioStream() || _eof;
}

int FCAudioStreamWriter::write(const FCFrame& frame)
{
	return FCStreamWriter::write(AVMEDIA_TYPE_AUDIO, frame);
}

int FCAudioStreamWriter::createFilter()
{
	auto dstStream = _muxer.audioStream();
	if (!dstStream)
	{
		return 0;
	}
	_filter.reset(new FCAudioFilter());
	auto srcStream = _demuxer->stream(_inStreamIndex);
	FCAudioFilterParameters params{};
	params.srcTimeBase = srcStream->time_base;
	params.srcSampleFormat = (AVSampleFormat)srcStream->codecpar->format;
	params.srcSampleRate = srcStream->codecpar->sample_rate;
	av_channel_layout_describe(&srcStream->codecpar->ch_layout, params.srcChannelLayout, sizeof(params.srcChannelLayout));
	params.dstSampleFormat = (AVSampleFormat)dstStream->codecpar->format;
	params.dstSampleRate = dstStream->codecpar->sample_rate;
	av_channel_layout_describe(&dstStream->codecpar->ch_layout, params.dstChannelLayout, sizeof(params.dstChannelLayout));
	params.filterString = _entry.aFilterString;
	params.frameSize = _muxer.fixedAudioFrameSize();
	return _filter->create(params);
}

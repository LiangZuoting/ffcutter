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
	auto filter = QSharedPointer<FCFilter>(new FCAudioFilter());
	_filters.append(filter);
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
	return filter->create(params);
}
#include "fcmuxer.h"
#include "fcutil.h"
#include "fcconst.h"

FCMuxer::~FCMuxer()
{
	destroy();
}

int FCMuxer::create(const FCMuxEntry &muxEntry)
{
	int ret = 0;
	do 
	{
		auto filePath = muxEntry.filePath.toStdString();
		int ret = avformat_alloc_output_context2(&_formatContext, nullptr, nullptr, filePath.data());
		if (ret < 0)
		{
			break;
		}

		_videoEncoder.reset(new FCVideoEncoder());
		ret = _videoEncoder->create(_formatContext, muxEntry);
		if (ret < 0)
		{
			break;
		}
		_audioEncoder.reset(new FCAudioEncoder());
		ret = _audioEncoder->create(_formatContext, muxEntry);
		if (ret < 0)
		{
			break;
		}

		ret = avio_open(&_formatContext->pb, filePath.data(), AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avio_open");
			break;
		}
		ret = avformat_write_header(_formatContext, nullptr);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avformat_write_header");
			break;
		}
	} while (0);

	return ret;
}

int FCMuxer::write(AVMediaType type, AVFrame *frame)
{
	int ret = 0;
	FCEncodeResult result;
	if (type == AVMEDIA_TYPE_VIDEO)
	{
		result = _videoEncoder->encode(frame);
	}
	else if (type == AVMEDIA_TYPE_AUDIO)
	{
		result = _audioEncoder->encode(frame);
	}
	ret = result.error;
	if (ret < 0)
	{
		return ret;
	}
	int count = 0;
	for (auto packet : result.packets)
	{
		ret = av_interleaved_write_frame(_formatContext, packet);
		av_packet_unref(packet);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "av_interleaved_write_frame");
			break;
		}
		++count;
	}
	if (ret >= 0)
	{
		ret = count;
	}
	return ret;
}

int FCMuxer::write(AVMediaType type, const QList<AVFrame *> &frames)
{
	int count = 0;
	for (auto &frame : frames)
	{
		int ret = write(type, frame);
		if (ret < 0)
		{
			return ret;
		}
		count += ret;
	}
	return count;
}

int FCMuxer::writeTrailer()
{
	int ret = av_write_trailer(_formatContext);
	if (ret < 0)
	{
		FCUtil::printAVError(ret, "av_write_trailer");
	}
	return ret;
}

AVStream *FCMuxer::audioStream() const
{
	return _audioEncoder->stream();
}

AVStream *FCMuxer::videoStream() const
{
	return _videoEncoder->stream();
}

AVPixelFormat FCMuxer::videoFormat() const
{
	if (_videoEncoder)
	{
		return (AVPixelFormat)_videoEncoder->format();
	}
	return AV_PIX_FMT_NONE;
}

int FCMuxer::fixedAudioFrameSize() const
{
	if (_audioEncoder)
	{
		return _audioEncoder->frameSize();
	}
	return 0;
}

void FCMuxer::destroy()
{
	if (_formatContext)
	{
		if (_formatContext->pb)
		{
			avio_close(_formatContext->pb);
		}
		avformat_free_context(_formatContext);
		_formatContext = nullptr;
	}
	_audioEncoder.reset();
	_videoEncoder.reset();
}
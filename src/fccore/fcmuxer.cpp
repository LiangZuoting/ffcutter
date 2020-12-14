#include "fcmuxer.h"
#include "fcutil.h"

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

		ret = createVideoCodec(muxEntry);
		if (ret)
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

	if (ret >= 0)
	{
		_encodedPacket = av_packet_alloc();
	}

	return ret;
}

int FCMuxer::writeVideo(const AVFrame *frame)
{
	int count = 0;
	int ret = avcodec_send_frame(_videoCodec, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		ret = 0;
	}
	else while (true)
	{
		ret = avcodec_receive_packet(_videoCodec, _encodedPacket);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			ret = 0;
			break;
		}
		if (!ret)
		{
			av_packet_rescale_ts(_encodedPacket, _videoCodec->time_base, _videoStream->time_base);
			ret = av_interleaved_write_frame(_formatContext, _encodedPacket);
			if (ret)
			{
				FCUtil::printAVError(ret, "av_interleaved_write_frame");
				break;
			}
			++count;
		}
		else
		{
			FCUtil::printAVError(ret, "avcodec_receive_packet");
			break;
		}
	}
	if (!ret)
	{
		ret = count;
	}
	return ret;
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

AVStream *FCMuxer::videoStream() const
{
	return _videoStream;
}

AVPixelFormat FCMuxer::videoFormat() const
{
	if (_videoCodec)
	{
		return _videoCodec->pix_fmt;
	}
	return AV_PIX_FMT_NONE;
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
	if (_videoCodec)
	{
		avcodec_free_context(&_videoCodec);
	}
	_videoStream = nullptr;
	if (_audioCodec)
	{
		avcodec_free_context(&_audioCodec);
	}
	if (_subtitleCodec)
	{
		avcodec_free_context(&_subtitleCodec);
	}
	if (_encodedPacket)
	{
		av_packet_unref(_encodedPacket);
		_encodedPacket = nullptr;
	}
}

int FCMuxer::createVideoCodec(const FCMuxEntry& muxEntry)
{
	int ret = 0;
	do 
	{
		if (auto codecId = _formatContext->oformat->video_codec; codecId != AV_CODEC_ID_NONE)
		{
			auto videoCodec = avcodec_find_encoder(codecId);
			_videoCodec = avcodec_alloc_context3(videoCodec);
			_videoCodec->time_base = { 1, muxEntry.fps };
			_videoCodec->width = muxEntry.width;
			_videoCodec->height = muxEntry.height;
			if (auto fmt = videoCodec->pix_fmts[0]; fmt != AV_PIX_FMT_NONE)
			{
				_videoCodec->pix_fmt = fmt;
			}
			_videoCodec->framerate = { 1, muxEntry.fps };
			/*
			* H264 codec 不能 set 这个 flag，否则文件不能解析；
			* gif 必须 set 这个 flag，否则图像效果不对。
			*/
			if (_formatContext->oformat->flags & AVFMT_GLOBALHEADER && _videoCodec->codec_id != AV_CODEC_ID_H264)
			{
				_videoCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}

			_videoStream = avformat_new_stream(_formatContext, videoCodec);
			_videoStream->id = _formatContext->nb_streams - 1;
			_videoStream->time_base = _videoCodec->time_base;
			_videoStream->avg_frame_rate = _videoCodec->framerate;
			int ret = avcodec_parameters_from_context(_videoStream->codecpar, _videoCodec);
			if (ret)
			{
				FCUtil::printAVError(ret, "avcodec_parameters_from_context");
				break;
			}
			ret = avcodec_open2(_videoCodec, videoCodec, nullptr);
			if (ret)
			{
				FCUtil::printAVError(ret, "avcodec_open2");
				break;
			}
		}
	} while (0);
	return ret;
}
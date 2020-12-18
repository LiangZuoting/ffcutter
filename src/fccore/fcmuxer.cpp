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

		ret = createVideoCodec(muxEntry);
		if (ret < 0)
		{
			break;
		}
		ret = createAudioCodec(muxEntry);
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

int FCMuxer::writeVideo(AVFrame *frame)
{
	if (frame)
	{
		frame->pts = _videoPts++;
	}
	return writeFrame(frame, _videoCodec, _videoStream);
}

int FCMuxer::writeVideos(const QList<AVFrame *> &frames)
{
	int ret = 0;
	int count = 0;
	for (auto &frame : frames)
	{
		ret = writeVideo(frame);
		if (ret < 0)
		{
			break;
		}
		count += ret;
	}
	if (ret >= 0)
	{
		ret = count;
	}
	return ret;
}

int FCMuxer::writeAudio(AVFrame *frame)
{
	if (frame)
	{
		frame->pts = av_rescale_q(_audioPts, { 1, _audioCodec->sample_rate }, _audioCodec->time_base);
		_audioPts += frame->nb_samples;
	}
	return writeFrame(frame, _audioCodec, _audioStream);
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
	return _audioStream;
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
			_videoCodec->bit_rate = muxEntry.vBitrate;
			_videoCodec->width = muxEntry.width;
			_videoCodec->height = muxEntry.height;
			_videoCodec->gop_size = muxEntry.gop;
			// 请求的格式优先，不支持时用第一个
			_videoCodec->pix_fmt = videoCodec->pix_fmts[0];
			for (int i = 0; ; ++i)
			{
				auto fmt = videoCodec->pix_fmts[i];
				if (fmt == AV_PIX_FMT_NONE)
				{
					break;
				}
				if (fmt == muxEntry.pixelFormat)
				{
					_videoCodec->pix_fmt = fmt;
					break;
				}
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

int FCMuxer::createAudioCodec(const FCMuxEntry &muxEntry)
{
	int ret = 0;
	if (muxEntry.aStreamIndex < 0 || _formatContext->oformat->audio_codec == AV_CODEC_ID_NONE)
	{
		return ret;
	}
	do 
	{
		AVCodec *audioCodec = avcodec_find_encoder(_formatContext->oformat->audio_codec);
		_audioCodec = avcodec_alloc_context3(audioCodec);
		_audioCodec->bit_rate = muxEntry.aBitrate;
		_audioCodec->sample_fmt = audioCodec->sample_fmts[0];
		for (int i = 0; ; ++i)
		{
			auto fmt = audioCodec->sample_fmts[i];
			if (fmt == AV_SAMPLE_FMT_NONE)
			{
				break;
			}
			if (fmt == muxEntry.sampleFormat)
			{
				_audioCodec->sample_fmt = fmt;
				break;
			}
		}
		_audioCodec->sample_rate = muxEntry.sampleRate;
		_audioCodec->channel_layout = muxEntry.channel_layout;
		for (int i = 0; ; ++i)
		{
			auto layout = audioCodec->channel_layouts[i];
			if (layout == -1)
			{
				break;
			}
			if (layout == muxEntry.channel_layout)
			{
				_audioCodec->channel_layout = layout;
				break;
			}
		}
		_audioCodec->channels = av_get_channel_layout_nb_channels(_audioCodec->channel_layout);
		if (_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
		{
			_audioCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}

		_audioStream = avformat_new_stream(_formatContext, audioCodec);
		_audioStream->id = _formatContext->nb_streams - 1;
		_audioStream->time_base = { 1, _audioCodec->sample_rate };
		ret = avcodec_parameters_from_context(_audioStream->codecpar, _audioCodec);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avcodec_parameters_from_context");
			break;
		}
		ret = avcodec_open2(_audioCodec, audioCodec, nullptr);
		if (ret < 0)
		{
			FCUtil::printAVError(ret, "avcodec_open2");
			break;
		}
	} while (0);
	return ret;
}

int FCMuxer::writeFrame(AVFrame *frame, AVCodecContext *codecContext, AVStream *stream)
{
	int count = 0;
	int ret = avcodec_send_frame(codecContext, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	{
		ret = 0;
	}
	else if (ret < 0)
	{
		FCUtil::printAVError(ret, "avcodec_send_frame");
	}
	else while (ret >= 0)
	{
		FCPacket packet = { 0 };
		ret = avcodec_receive_packet(codecContext, &packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			ret = 0;
			break;
		}
		if (!ret)
		{
			av_packet_rescale_ts(&packet, codecContext->time_base, stream->time_base);
			packet.stream_index = stream->index;
			ret = av_interleaved_write_frame(_formatContext, &packet);
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
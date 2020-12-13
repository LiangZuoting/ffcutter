#pragma once

#include "fcmuxentry.h"
#include <QPair>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

/// <summary>
/// encoding and then muxing
/// </summary>
class FCMuxer
{
public:
	~FCMuxer();

	/// <summary>
	/// 
	/// </summary>
	/// <param name="muxEntry"></param>
	/// <returns>����Ϊ�����룬����ɹ�</returns>
	int create(const FCMuxEntry &muxEntry);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="frame"></param>
	/// <returns>����Ϊ�����룬����Ϊд���֡��</returns>
	int writeVideo(const AVFrame *frame);

	int writeTrailer();

	AVPixelFormat videoFormat() const;

	void destroy();

private:
	int createVideoCodec(const FCMuxEntry& muxEntry);

	AVFormatContext *_formatContext = nullptr;
	AVCodecContext *_audioCodec = nullptr;
	AVCodecContext *_videoCodec = nullptr;
	AVStream* _videoStream = nullptr;
	AVCodecContext *_subtitleCodec = nullptr;
	AVPacket* _encodedPacket = nullptr;
};
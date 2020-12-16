#pragma once

#include <QPair>
#include "fcmuxentry.h"
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
	int writeVideo(AVFrame *frame);
	int writeAudio(AVFrame *frame);

	int writeTrailer();

	AVStream *videoStream() const;
	AVPixelFormat videoFormat() const;

	void destroy();

private:
	int createVideoCodec(const FCMuxEntry& muxEntry);
	int createAudioCodec(const FCMuxEntry &muxEntry);
	int writeFrame(AVFrame *frame, AVMediaType mediaType);

	AVFormatContext *_formatContext = nullptr;
	AVCodecContext *_audioCodec = nullptr;
	AVStream *_audioStream = nullptr;
	AVCodecContext *_videoCodec = nullptr;
	AVStream* _videoStream = nullptr;
	AVCodecContext *_subtitleCodec = nullptr;
	AVPacket* _encodedPacket = nullptr;
	int64_t _sampleCount = 0;
};
#pragma once

#include <QSharedPointer>
#include "fcvideoencoder.h"
#include "fcaudioencoder.h"
#include "fcconst.h"
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
	/// <returns>负数为错误码，否则成功</returns>
	int create(const FCMuxEntry &muxEntry);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="type"></param>
	/// <param name="frame"></param>
	/// <returns>负数为错误码，否则为写入的帧数</returns>
	int write(AVMediaType type, AVFrame *frame);
	int write(AVMediaType type, const QList<AVFrame *> &frames);

	int writeTrailer();

	AVStream *audioStream() const;
	AVStream *videoStream() const;
	AVPixelFormat videoFormat() const;

	int fixedAudioFrameSize() const;

	void destroy();

private:
	AVFormatContext *_formatContext = nullptr;
	QSharedPointer<FCEncoder> _audioEncoder;
	QSharedPointer<FCEncoder> _videoEncoder;
};
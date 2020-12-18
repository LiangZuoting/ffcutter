#pragma once

#include <QMap>
#include <QPair>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include "fcconst.h"

/// <summary>
/// demuxing and then decoding
/// </summary>
class FCDemuxer
{
public:
	~FCDemuxer();

	int open(const QString& filePath);

	/// <summary>
	/// seek 到后边第一个关键帧
	/// </summary>
	/// <param name="streamIndex"></param>
	/// <param name="timestamp"></param>
	/// <returns></returns>
	int fastSeek(int streamIndex, int64_t timestamp);
	/// <summary>
	/// seek 到时间戳前边的第一帧
	/// 会从后边第一个关键帧一直解码到时间戳处
	/// </summary>
	/// <param name="streamIndex"></param>
	/// <param name="timestamp"></param>
	/// <returns></returns>
	int exactSeek(int streamIndex, int64_t timestamp);
	/// <summary>
	/// 解码下一帧
	/// </summary>
	/// <param name="streamFilter">
	/// 如果不为空，只解码指定流中的帧；否则解码所有流中的帧
	/// </param>
	/// <returns>负数为错误码，否则 second 包含解码后帧数据</returns>
	FCDecodeResult decodeNextPacket(const QVector<int>& streamFilter);

	AVFormatContext* formatContext() const;
	QList<AVStream*> streams() const;
	AVStream* stream(int streamIndex) const;

	double tsToSec(int streamIndex, int64_t timestamp) const;
	int64_t secToTs(int streamIndex, double seconds) const;

	void close();

private:
	QPair<int, AVCodecContext*> getCodecContext(int streamIndex);
	FCDecodeResult decodePacket(int streamIndex, AVPacket *packet);

	AVFormatContext* _formatContext = nullptr;
	// map from stream index to codec context
	QMap<int, AVCodecContext*> _codecContexts;
	QMap<int, AVStream*> _streams;
};
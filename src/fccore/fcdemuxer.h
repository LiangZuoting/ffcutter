#pragma once

#include <QMap>
#include <QPair>
#include <QSharedPointer>
extern "C"
{
#include <libavformat/avformat.h>
}
#include "fcdecoder.h"
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
	/// seek ����ߵ�һ���ؼ�֡
	/// </summary>
	/// <param name="streamIndex"></param>
	/// <param name="timestamp"></param>
	/// <returns></returns>
	int fastSeek(int streamIndex, int64_t timestamp);
	/// <summary>
	/// seek ��ʱ���ǰ�ߵĵ�һ֡
	/// ��Ӻ�ߵ�һ���ؼ�֡һֱ���뵽ʱ�����
	/// </summary>
	/// <param name="streamIndex"></param>
	/// <param name="timestamp"></param>
	/// <returns></returns>
	FCDecodeResult exactSeek(int streamIndex, int64_t timestamp);
	/// <summary>
	/// ������һ֡
	/// </summary>
	/// <param name="streamFilter">
	/// �����Ϊ�գ�ֻ����ָ�����е�֡����������������е�֡
	/// </param>
	/// <returns>����Ϊ�����룬���� second ���������֡����</returns>
	FCDecodeResult decodeNextPacket(const QVector<int>& streamFilter);

	AVFormatContext* formatContext() const;
	QList<AVStream*> streams() const;
	AVStream* stream(int streamIndex) const;

	double tsToSec(int streamIndex, int64_t timestamp) const;
	int64_t secToTs(int streamIndex, double seconds) const;

	void close();

private:
	QPair<int, QSharedPointer<FCDecoder>> getCodecContext(int streamIndex);
	FCDecodeResult decodePacket(int streamIndex, AVPacket *packet);

	AVFormatContext* _formatContext = nullptr;
	// map from stream index to codec context
	QMap<int, QSharedPointer<FCDecoder>> _decoders;
	QMap<int, AVStream*> _streams;
};
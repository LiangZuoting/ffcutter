#pragma once

#include <QMap>
#include <QPair>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

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
	int exactSeek(int streamIndex, int64_t timestamp);
	/// <summary>
	/// ������һ֡
	/// </summary>
	/// <param name="streamFilter">
	/// �����Ϊ�գ�ֻ����ָ�����е�֡����������������е�֡
	/// </param>
	/// <returns>����Ϊ�����룬���� second ���������֡����</returns>
	QPair<int, QList<AVFrame*>> decodeNextPacket(const QVector<int>& streamFilter);

	AVFormatContext* formatContext() const;
	QList<AVStream*> streams() const;
	AVStream* stream(int streamIndex) const;

	double tsToSecond(int streamIndex, int64_t timestamp) const;
	int64_t secondToTs(int streamIndex, double seconds) const;

	void close();

private:
	QPair<int, AVCodecContext*> getCodecContext(int streamIndex);

	AVFormatContext* _formatContext = nullptr;
	// map from stream index to codec context
	QMap<int, AVCodecContext*> _codecContexts;
	QMap<int, AVStream*> _streams;
	AVPacket* _demuxedPacket = nullptr;
};
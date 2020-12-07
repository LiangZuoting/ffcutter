#pragma once

#include "fccore_global.h"
#include <QThreadPool>
#include <QMap>
#include <QFuture>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}


class FCCORE_EXPORT FCService
{
public:
    inline static const int DEMUX_INDEX = -1;

    FCService();

    /// <summary>
    /// 不可重入
    /// </summary>
    /// <param name="filePath"></param>
    /// <returns></returns>
    QFuture<QList<AVStream*>> openFile(const QString& filePath);
    QFuture<AVFrame*> decodeOneFrame(int streamIndex);
    QFuture<QVector<AVFrame*>> decodeFrames(int streamIndex, int count);

    AVFormatContext* formatContext() const;
    AVStream* stream(int streamIndex) const;
    QList<AVStream*> streams() const;

    QFuture<void> seek(int streamIndex, int64_t timestamp);

    QPair<int, QString> lastError();

private:
    inline AVFrame* decodeNextFrame(int streamIndex);
    QThreadPool* getThreadPool(int streamIndex);
    AVCodecContext* getCodecContext(int streamIndex);
    AVPacket* getPacket(int streamIndex);

    int _lastError = 0;
    QString _lastErrorString;
    /// <summary>
	/// map from stream index to thread pool
	/// key = -1 for demuxing thread
    /// </summary>
    QMap<int, QThreadPool*> _mapFromIndexToThread;
    AVFormatContext* _formatContext = nullptr;
    /// <summary>
    /// map from stream index to codec context
    /// </summary>
    QMap<int, AVCodecContext*> _mapFromIndexToCodec;
    /// <summary>
    /// map from stream index to stream struct
    /// </summary>
    QMap<int, AVStream*> _mapFromIndexToStream;
    QMap<int, AVPacket*> _mapFromIndexToPacket;
};

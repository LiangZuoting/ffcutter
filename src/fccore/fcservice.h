#pragma once

#include "fccore_global.h"
#include <QObject>
#include <QThreadPool>
#include <QMap>
#include <QPixmap>
#include <QMutex>
#include "fcscaler.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

Q_DECLARE_METATYPE(QList<AVStream*>)

class FCCORE_EXPORT FCService : public QObject
{
    Q_OBJECT

public:
    inline static const int DEMUX_INDEX = -1;

    FCService();
    ~FCService();

    /// <summary>
    /// 不可重入
    /// </summary>
    /// <param name="filePath"></param>
    /// <returns></returns>
    void openFileAsync(const QString& filePath);
    void decodeOneFrameAsync(int streamIndex);
    void decodeFramesAsync(int streamIndex, int count);

    AVFormatContext* formatContext() const;
    AVStream* stream(int streamIndex) const;
    QList<AVStream*> streams() const;

    void seekAsync(int streamIndex, double timestampInSecond);

    void scaleAsync(AVFrame* frame, int destWidth, int destHeight);

    QPair<int, QString> lastError();

    double timestampToSecond(int streamIndex, int64_t timestamp);

    void destroy();

Q_SIGNALS:
    void fileOpened(QList<AVStream *>);
    void frameDeocded(AVFrame *);
    void decodeFinished();
    void scaleFinished(QPixmap);
    void seekFinished();

private:
    inline AVFrame* decodeNextFrame(int streamIndex);
    QThreadPool* getThreadPool(int streamIndex);
    AVCodecContext* getCodecContext(int streamIndex);
    AVPacket* getPacket(int streamIndex);
    QSharedPointer<FCScaler> getScaler(AVFrame *frame, int destWidth, int destHeight, AVPixelFormat destFormat);

    QMutex _mutex;
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
    QVector<QSharedPointer<FCScaler>> _vecScaler;
    QMutex _scaleMutex;
};

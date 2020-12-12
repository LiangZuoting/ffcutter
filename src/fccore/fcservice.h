#pragma once

#include "fccore_global.h"
#include <QObject>
#include <QThreadPool>
#include <QMap>
#include <QPixmap>
#include <QMutex>
#include "fcscaler.h"
#include "fcmuxentry.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

Q_DECLARE_METATYPE(QList<AVStream*>)
Q_DECLARE_METATYPE(QList<AVFrame*>)

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
    void decodeOnePacketAsync(int streamIndex);
    void decodePacketsAsync(int streamIndex, int count);

    AVFormatContext* formatContext() const;
    AVStream* stream(int streamIndex) const;
    QList<AVStream*> streams() const;

    void seekAsync(int streamIndex, double timestampInSecond);

    void scaleAsync(AVFrame* frame, AVPixelFormat destFormat, int destWidth, int destHeight);

    void saveAsync(const FCMuxEntry& entry);

    QPair<int, QString> lastError();

    double timestampToSecond(int streamIndex, int64_t timestamp);

    void destroy();

Q_SIGNALS:
    void errorOcurred();
    void fileOpened(QList<AVStream *>);
    void frameDeocded(QList<AVFrame*>);
    void decodeFinished();
    void scaleFinished(QPixmap);
    void seekFinished();
    void saveFinished();

private:
    bool seek(int streamIndex, int64_t timestamp);
    FCScaler::ScaleResult scale(AVFrame *frame, AVPixelFormat destFormat, int destWidth, int destHeight, uint8_t *scaledData[4] = nullptr, int scaledLineSizes[4] = nullptr);
    QList<AVFrame *> decodeNextPacket(const QVector<int> &streamFilter);
    AVCodecContext* getCodecContext(int streamIndex);
    AVPacket* getPacket();
    QSharedPointer<FCScaler> getScaler(AVFrame *frame, int destWidth, int destHeight, AVPixelFormat destFormat);

    QMutex _mutex;
    int _lastError = 0;
    QString _lastErrorString;
    QThreadPool *_threadPool = nullptr;
    AVFormatContext* _formatContext = nullptr;
    /// <summary>
    /// map from stream index to codec context
    /// </summary>
    QMap<int, AVCodecContext*> _mapFromIndexToCodec;
    /// <summary>
    /// map from stream index to stream struct
    /// </summary>
    QMap<int, AVStream*> _mapFromIndexToStream;
    AVPacket *_readPacket = nullptr;
    QVector<QSharedPointer<FCScaler>> _vecScaler;
    QMutex _scaleMutex;
};

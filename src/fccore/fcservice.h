#pragma once

#include "fccore_global.h"
#include <QObject>
#include <QPair>
#include <QThreadPool>
#include <QMap>
#include <QPixmap>
#include <QMutex>
#include "fcscaler.h"
#include "fcmuxentry.h"
#include "fcdemuxer.h"
#include "fcfilter.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

Q_DECLARE_METATYPE(QList<AVStream*>)
Q_DECLARE_METATYPE(QList<FCFrame>)

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

    void seekAsync(int streamIndex, double seconds);
    /// <summary>
    /// 将视频 frame 缩放到指定分辨率，并转换成 RGB24 格式的 Pixmap
    /// </summary>
    /// <param name="frame"></param>
    /// <param name="destWidth"></param>
    /// <param name="destHeight"></param>
    void scaleAsync(AVFrame* frame, int destWidth, int destHeight);

    void saveAsync(const FCMuxEntry &muxEntry);

    QPair<int, QString> lastError();

    double tsToSec(int streamIndex, int64_t timestamp);

    void destroy();

Q_SIGNALS:
    void eof();
    void errorOcurred();
    void fileOpened(QList<AVStream *>);
    void frameDeocded(QList<FCFrame>);
    void decodeFinished();
    void scaleFinished(AVFrame *src, QPixmap scaled);
    void seekFinished(int streamIndex);
    void saveFinished();

private:
    FCScaler::ScaleResult scale(AVFrame *frame, AVPixelFormat destFormat, int destWidth, int destHeight, uint8_t *scaledData[4] = nullptr, int scaledLineSizes[4] = nullptr);
    QSharedPointer<FCScaler> getScaler(AVFrame *frame, int destWidth, int destHeight, AVPixelFormat destFormat);
    void clearFrames(QList<AVFrame *> &frames);
    void clearFrames(QList<FCFrame> &frames);
    QSharedPointer<FCFilter> createVideoFilter(const AVStream *srcStream, QString filters, AVPixelFormat dstPixelFormat);

    QMutex _mutex;
    int _lastError = 0;
    QString _lastErrorString;
    QThreadPool *_threadPool = nullptr;
    FCDemuxer* _demuxer = nullptr;
    QVector<QSharedPointer<FCScaler>> _vecScaler;
};

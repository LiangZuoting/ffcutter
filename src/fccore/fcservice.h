#pragma once

#include "fccore_global.h"
#include <QObject>
#include <QPair>
#include <QThreadPool>
#include <QMap>
#include <QPixmap>
#include <QMutex>
#include "fcscaler.h"
#include "fcmuxer.h"
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

    void fastSeekAsync(int streamIndex, double seconds);
    void exactSeekAsync(int streamIndex, double seconds);
    /// <summary>
    /// 将视频 frame 缩放到指定分辨率，并转换成 RGB24 格式的 Pixmap
    /// </summary>
    /// <param name="frame"></param>
    /// <param name="dstWidth"></param>
    /// <param name="dstHeight"></param>
    void scaleAsync(AVFrame* frame, int dstWidth, int dstHeight);

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
    void seekFinished(int streamIndex, QList<FCFrame> frames);
    void saveFinished();

private:
    FCScaler::ScaleResult scale(AVFrame *frame, AVPixelFormat dstFormat, int dstWidth, int dstHeight, uint8_t *scaledData[4] = nullptr, int scaledLineSizes[4] = nullptr);
    QSharedPointer<FCScaler> getScaler(AVFrame *frame, int dstWidth, int dstHeight, AVPixelFormat dstFormat);
    void clearFrames(QList<AVFrame *> &frames);
    void clearFrames(QList<FCFrame> &frames);
    QSharedPointer<FCFilter> createVideoFilter(const AVStream *srcStream, QString filters, AVPixelFormat dstPixelFormat);
    QSharedPointer<FCFilter> createAudioFilter(const AVStream* srcStream, QString filters, const AVStream* dstStream, int frameSize);
    bool filterAndMuxFrame(QSharedPointer<FCFilter>& filter, FCMuxer& muxer, AVFrame* frame);

    QMutex _mutex;
    int _lastError = 0;
    QString _lastErrorString;
    QThreadPool *_threadPool = nullptr;
    FCDemuxer* _demuxer = nullptr;
    QVector<QSharedPointer<FCScaler>> _vecScaler;
};

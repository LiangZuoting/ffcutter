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
    void openFileAsync(const QString& filePath, void *userData);
    void decodeOnePacketAsync(int streamIndex, void *userData);
    void decodePacketsAsync(int streamIndex, int count, void *userData);

    AVFormatContext* formatContext() const;
    AVStream* stream(int streamIndex) const;
    QList<AVStream*> streams() const;

    void fastSeekAsync(int streamIndex, double seconds, void *userData);
    void exactSeekAsync(int streamIndex, double seconds, void *userData);
    /// <summary>
    /// 将视频 frame 缩放到指定分辨率，并转换成 RGB24 格式的 Pixmap
    /// </summary>
    /// <param name="frame"></param>
    /// <param name="dstWidth"></param>
    /// <param name="dstHeight"></param>
    void scaleAsync(AVFrame* frame, int dstWidth, int dstHeight, void *userData);

    void saveAsync(const FCMuxEntry &muxEntry, void *userData);

    QPair<int, QString> lastError();

    double tsToSec(int streamIndex, int64_t timestamp);

    void destroy();

Q_SIGNALS:
    void eof(void *userData);
    void errorOcurred(void *userData);
    void fileOpened(QList<AVStream *>, void *userData);
    void frameDeocded(QList<FCFrame>, void *);
    void decodeFinished(void *userData);
    void scaleFinished(AVFrame *src, QPixmap scaled, void *userData);
    void seekFinished(int streamIndex, QList<FCFrame> frames, void *userData);
    void saveFinished(void *userData);

private:
    FCScaler::ScaleResult scale(AVFrame *frame, AVPixelFormat dstFormat, int dstWidth, int dstHeight, uint8_t *scaledData[4] = nullptr, int scaledLineSizes[4] = nullptr);
    QSharedPointer<FCScaler> getScaler(AVFrame *frame, int dstWidth, int dstHeight, AVPixelFormat dstFormat);
    void clearFrames(QList<FCFrame> &frames);

    QMutex _mutex;
    int _lastError = 0;
    QString _lastErrorString;
    QThreadPool *_threadPool = nullptr;
    QSharedPointer<FCDemuxer> _demuxer = nullptr;
    QVector<QSharedPointer<FCScaler>> _vecScaler;
};

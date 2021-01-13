#pragma once

#include <QSharedPointer>
#include "fccore_global.h"
#include "fcdemuxer.h"
#include "fcmuxer.h"
#include "fcfilter.h"
#include "fcconst.h"

class FCStreamWriter
{
public:
    FCStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer);
    virtual int create() = 0;
    virtual bool eof() const = 0;
    void setEof();
    virtual int write(const FCFrame& frame) = 0;
    
protected:
    int write(AVMediaType type, const FCFrame& frame);
    int checkPtsRange(const FCFrame& frame);
    void clearFrames(QList<AVFrame*>& frames);

    bool _eof = false;
    int _inStreamIndex = -1;
    int64_t _startPts = 0;
    int64_t _endPts = INT64_MAX;
    const FCMuxEntry& _entry;
    QSharedPointer<FCDemuxer> _demuxer;
    FCMuxer& _muxer;
    QList<QSharedPointer<FCFilter>> _filters;
    int _currentFilter = 0;
};

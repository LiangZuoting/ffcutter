#pragma once

#include <QSharedPointer>
#include "fccore_global.h"
#include "fcmuxentry.h"
#include "fcdemuxer.h"
#include "fcmuxer.h"
#include "fcfilter.h"

class FCVideoStreamWriter
{
public:
    FCVideoStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer);
    int create();
    bool eof() const;
    void setEof();
    int write(const FCFrame& frame);
    
private:
    int createFilter();
    int checkPtsRange(const FCFrame& frame);
    void clearFrames(QList<AVFrame*>& frames);

    bool _eof = false;
    int _inStreamIndex = -1;
    int64_t _startPts = 0;
    int64_t _endPts = INT64_MAX;
    const FCMuxEntry& _entry;
    QSharedPointer<FCDemuxer> _demuxer;
    FCMuxer& _muxer;
    QSharedPointer<FCFilter> _filter;
};

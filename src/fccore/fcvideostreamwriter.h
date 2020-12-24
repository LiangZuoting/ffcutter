#pragma once

#include "fcstreamwriter.h"

class FCVideoStreamWriter : public FCStreamWriter
{
public:
    FCVideoStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer);
    int create() override;
    bool eof() const override;
    int write(const FCFrame& frame) override;
    
private:
    int createFilter();
};

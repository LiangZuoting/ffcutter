#pragma once

#include "fcstreamwriter.h"

class FCAudioStreamWriter : public FCStreamWriter
{
public:
    FCAudioStreamWriter(const FCMuxEntry& entry, const QSharedPointer<FCDemuxer>& demuxer, FCMuxer& muxer);
    int create() override;
    bool eof() const override;
    int write(const FCFrame& frame) override;
    
private:
    int createFilter();
};

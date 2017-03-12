#ifndef OZ_ALARM_FRAME_H
#define OZ_ALARM_FRAME_H

#include "ozFeedFrame.h"

///
/// Specialised video frame that includes an indication of whether this frame is alarmed or not.
///
class AlarmFrame : public VideoFrame
{
protected:
    bool mAlarmed;      ///< Indicates whether this frame represents an alarmed frame.

public:
    AlarmFrame( VideoProvider *provider, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, bool alarmed ) :
        VideoFrame( provider, id, timestamp, buffer ),
        mAlarmed( alarmed )
    {
    }
    //AlarmFrame( VideoProvider *provider, uint64_t id, uint64_t timestamp, const uint8_t *buffer, size_t size );
    AlarmFrame( VideoProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, bool alarmed ) :
        VideoFrame( provider, parent, id, timestamp, buffer ),
        mAlarmed( alarmed )
    {
    }
    /// Create a new frame from a data pointer and relate it to a parent frame
    AlarmFrame( VideoProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const uint8_t *buffer, size_t size, bool alarmed ) :
        VideoFrame( provider, parent, id, timestamp, buffer, size ),
        mAlarmed( alarmed )
    {
    }
    AlarmFrame( VideoProvider *provider, const FramePtr &parent, bool alarmed ) :
        VideoFrame( provider, parent ),
        mAlarmed( alarmed )
    {
    }

    bool alarmed() const { return( mAlarmed ); }
};

#endif // OZ_ALARM_FRAME_H

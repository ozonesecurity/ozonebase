#ifndef ZM_MOTION_FRAME_H
#define ZM_MOTION_FRAME_H

#include "zmAlarmFrame.h"
#include "zmMotionData.h"
#include "processors/zmMotionDetector.h"

///
/// Specialised alarm frame that includes a reference to a motion detector and information on the motion that was detected.
///
class MotionFrame : public AlarmFrame
{
protected:
    MotionDetector  *mMotionDetector;       ///< Pointer to the motion detector that generated this frame
    const MotionData      *mMotionData;     ///< Pointer to motion data describing motion objects in this frame

public:
    MotionFrame( MotionDetector *provider, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, bool alarmed, const MotionData *motionData ) :
        AlarmFrame( provider, id, timestamp, buffer, alarmed ),
        mMotionDetector( provider ),
        mMotionData( motionData )
    {
    }
    MotionFrame( MotionDetector *provider, FramePtr parent, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, bool alarmed, const MotionData *motionData ) :
        AlarmFrame( provider, parent, id, timestamp, buffer, alarmed ),
        mMotionDetector( provider ),
        mMotionData( motionData )
    {
    }
    MotionFrame( MotionDetector *provider, FramePtr parent, bool alarmed, const MotionData *motionData ) :
        AlarmFrame( provider, parent, alarmed ),
        mMotionDetector( provider ),
        mMotionData( motionData )
    {
    }
    ~MotionFrame()
    {
        delete mMotionData;
    }

    const MotionDetector *motionDetector() const { return( mMotionDetector ); }
    const MotionData *motionData() const { return( mMotionData ); }
};

#endif // ZM_MOTION_FRAME_H

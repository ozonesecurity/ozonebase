#ifndef OZ_MOTION_FRAME_H
#define OZ_MOTION_FRAME_H

#include "ozAlarmFrame.h"
#include "ozMotionData.h"

class MotionDetector;

///
/// Specialised alarm frame that includes a reference to a motion detector and information on the motion that was detected.
///
class MotionFrame : public AlarmFrame
{
protected:
    MotionDetector      *mMotionDetector; ///< Pointer to the motion detector that generated this frame
    const MotionData    *mMotionData;     ///< Pointer to motion data describing motion objects in this frame

public:
    MotionFrame( MotionDetector *provider, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, bool alarmed, const MotionData *motionData );
    MotionFrame( MotionDetector *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, bool alarmed, const MotionData *motionData );
    MotionFrame( MotionDetector *provider, const FramePtr &parent, bool alarmed, const MotionData *motionData );

    ~MotionFrame();

    const MotionDetector *motionDetector() const { return( mMotionDetector ); }
    const MotionData *motionData() const { return( mMotionData ); }
};

#endif // OZ_MOTION_FRAME_H

#include "oz.h"
#include "ozMotionFrame.h"

#include "ozZone.h"
#include "../processors/ozMotionDetector.h"

MotionFrame::MotionFrame( MotionDetector *provider, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, bool alarmed, const MotionData *motionData ) :
    AlarmFrame( provider, id, timestamp, buffer, alarmed ),
    mMotionDetector( provider ),
    mMotionData( motionData )
{
}

MotionFrame::MotionFrame( MotionDetector *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, bool alarmed, const MotionData *motionData ) :
    AlarmFrame( provider, parent, id, timestamp, buffer, alarmed ),
    mMotionDetector( provider ),
    mMotionData( motionData )
{
}

MotionFrame::MotionFrame( MotionDetector *provider, const FramePtr &parent, bool alarmed, const MotionData *motionData ) :
    AlarmFrame( provider, parent, alarmed ),
    mMotionDetector( provider ),
    mMotionData( motionData )
{
}

MotionFrame::~MotionFrame()
{
    delete mMotionData;
}

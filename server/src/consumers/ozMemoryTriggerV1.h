/** @addtogroup Consumers */
/*@{*/
#ifndef OZ_MEMORY_TRIGGER_V1_H
#define OZ_MEMORY_TRIGGER_V1_H

#include "../base/ozFeedConsumer.h"
#include "../base/ozMemoryIOV1.h"
#include "../libgen/libgenThread.h"

class Image;

///
/// Consumer class that receives alarm frames and writes to the trigger region of 
/// traditional ZoneMinder shared memory.
///
class MemoryTriggerV1 : public VideoConsumer, public MemoryIOV1, public Thread
{
CLASSID(MemoryTriggerV1);

protected:
    static const int MAX_EVENT_HEAD_AGE = 3;    ///< Number of seconds of video before event to save
    static const int MAX_EVENT_TAIL_AGE = 3;    ///< Number of seconds of video after event to save

protected:
    typedef enum { IDLE, PREALARM, ALARM, ALERT } AlarmState;

protected:
    AlarmState      mState;
    int             mEventCount;                ///< Temp, in lieu of DB event index
    int             mNotificationId;
    uint64_t        mAlarmTime;
	double          mMinTime;
    uint64_t    	mImageCount;

protected:
    int run();
    bool processFrame( const FramePtr &frame );

public:
    MemoryTriggerV1( const std::string &name, const std::string &location, int memoryKey, double minTime=0 );
    ~MemoryTriggerV1();
};

#endif // OZ_MEMORY_TRIGGER_V1_H
/*@}*/

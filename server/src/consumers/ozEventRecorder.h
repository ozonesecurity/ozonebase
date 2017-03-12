/** @addtogroup Consumers */
/*@{*/

#ifndef OZ_EVENT_RECORDER_H
#define OZ_EVENT_RECORDER_H

#include "../base/ozFeedConsumer.h"
#include "../libgen/libgenThread.h"

#include "../base/ozMotionFrame.h"
#include "../base/ozRecorder.h"

#include <deque>


///
/// Consumer class used to record video stream for which motion or other significant
/// activity has been detected. Ultimately will write to a DB, but for now just to filesystem.
///
class EventRecorder : public virtual Recorder
{
CLASSID(EventRecorder);

protected:
    static const int MAX_EVENT_HEAD_AGE = 2;    ///< Number of seconds of video before event to save
    static const int MAX_EVENT_TAIL_AGE = 3;    ///< Number of seconds of video after event to save

protected:
    typedef std::deque<FramePtr> FrameStore;
    typedef enum { IDLE, PREALARM, ALARM, ALERT } AlarmState;

protected:
    std::string     mLocation;                  ///< Where to store the saved files
    FrameStore      mFrameStore;
    AlarmState      mState;
    int             mFrameCount;
    int             mEventCount;                ///< Temp, in lieu of DB event index
    uint64_t        mAlarmTime;
    double          mMinTime;

protected:
    int run();
    bool processFrame( const FramePtr & );

public:
/**
* @brief 
*
* @param name Name of instance
* @param location location base path where to store videos
* @param minTime    Minimum amount of time (in seconds) to record after an alarm is triggered

\code
nvrcam.event = new EventRecorder( "event-"+name,  path,30);
\endcode
*/
    EventRecorder( const std::string &name, const std::string &location, double minTime=0 ) :
        VideoConsumer( cClass(), name, 5 ),
        Thread( identity() ),
        mLocation( location ),
        mState( IDLE ),
        mFrameCount( 0 ),
        mEventCount( 0 ),
        mAlarmTime( 0 ),
        mMinTime (minTime)
    {
    }
    ~EventRecorder()
    {
    }
};

#endif // OZ_EVENT_RECORDER_H

/*@}*/

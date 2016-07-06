/** @addtogroup Consumers */
/*@{*/

#ifndef OZ_EVENT_DETECTOR_H
#define OZ_EVENT_DETECTOR_H

#include <base/ozFeedConsumer.h>
#include <libgen/libgenThread.h>

#include <base/ozMotionFrame.h>

#include <deque>
#include <functional>


///
/// Consumer class used to record video stream for which motion or other significant
/// activity has been detected. Ultimately will write to a DB, but for now just to filesystem.
///
class EventDetector : public VideoConsumer, public Thread
{
CLASSID(EventDetector);

protected:
    static const int MAX_EVENT_HEAD_AGE = 2;    ///< Number of seconds of video before event to save
    static const int MAX_EVENT_TAIL_AGE = 3;    ///< Number of seconds of video after event to save

protected:
    typedef std::deque<FramePtr> FrameStore;
    typedef enum { IDLE, PREALARM, ALARM, ALERT } AlarmState;

protected:
	std::function< void(int) > mFunction;
    std::string     mLocation;                  ///< Where to store the saved files
    FrameStore      mFrameStore;
    AlarmState      mState;
    int             mFrameCount;
    int             mEventCount;                ///< Temp, in lieu of DB event index
    uint64_t        mAlarmTime;

protected:
    int run();
    bool processFrame( FramePtr );

public:
    EventDetector( const std::string &name, std::function<void(int)> mFunc ) :
        VideoConsumer( cClass(), name, 5 ),
        Thread( identity() ),
        mState( IDLE ),
        mFrameCount( 0 ),
        mEventCount( 0 ),
        mAlarmTime( 0 ),
		mFunction(mFunc)
    {
    }
     EventDetector( const std::string &name, const std::string &location ) :
        VideoConsumer( cClass(), name, 5 ),
        Thread( identity() ),
        mLocation( location ),
        mState( IDLE ),
        mFrameCount( 0 ),
        mEventCount( 0 ),
        mAlarmTime( 0 )
    {
    }
    ~EventDetector()
    {
    }
};

#endif // OZ_EVENT_DETECTOR_H

/*@}*/

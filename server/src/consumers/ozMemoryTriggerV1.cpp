#include "../base/oz.h"
#include "ozMemoryTriggerV1.h"

#include "../base/ozAlarmFrame.h"
#include "../base/ozFeedProvider.h"
#include "../libgen/libgenTime.h"

//#include <sys/types.h>
#include <sys/stat.h>
//#include <arpa/inet.h>
//#include <glob.h>

#if OZ_MEM_MAPPED
#include <sys/mman.h>
#include <fcntl.h>
#else // OZ_MEM_MAPPED
#include <sys/ipc.h>
#include <sys/shm.h>
#endif // OZ_MEM_MAPPED

/**
* @brief 
*
* @param name
* @param location
* @param memoryKey
*/
MemoryTriggerV1::MemoryTriggerV1( const std::string &name, const std::string &location, int memoryKey, double minTime ) :
    VideoConsumer( cClass(), name, 5 ),
    MemoryIOV1( location, memoryKey, true ),
    Thread( identity() ),
    mState( IDLE ),
    mEventCount( 0 ),
    mNotificationId( 0 ),
    mAlarmTime( 0 ),
	mMinTime( minTime ),
    mImageCount( 0 )
{
}

/**
* @brief 
*/
MemoryTriggerV1::~MemoryTriggerV1()
{
}

/**
* @brief 
*
* @return 
*/
int MemoryTriggerV1::run()
{
    if ( waitForProviders() )
    {
        attachMemory();
        while( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    processFrame( *iter );
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    cleanup();
    return( 0 );
}

bool MemoryTriggerV1::processFrame( const FramePtr &frame )
{
    const AlarmFrame *alarmFrame = dynamic_cast<const AlarmFrame *>(frame.get());
    static uint64_t mLastAlarmTime;

    if ( !alarmFrame )
        return( false );

    AlarmState lastState = mState;

    if ( alarmFrame->alarmed() )
    {
        mState = ALARM;
        mLastAlarmTime = time64();

        if ( lastState == IDLE )
        {
            // Create new event
            mAlarmTime = mLastAlarmTime;
            mEventCount++;
            setTrigger( 100, "oZone Detector", "Event detected by "+alarmFrame->provider()->identity() );
            //EventNotification::EventDetail detail( mEventCount, EventNotification::EventDetail::BEGIN );
            //EventNotification *notification = new EventNotification( this, alarmFrame->id(), detail );
        }
    }
    else if ( lastState == ALARM )
    {
        mState = ALERT;
    }

    if ( mState == ALERT )
    {
        if ( frame->age( mLastAlarmTime ) < -MAX_EVENT_TAIL_AGE )
        {
            if ((((double)mLastAlarmTime-mAlarmTime)/1000000.0) >= mMinTime)
            {
                mState = IDLE;
                clearTrigger();
                //EventNotification::EventDetail detail( mEventCount, ((double)mLastAlarmTime-mAlarmTime)/1000000.0 );
                //EventNotification *notification = new EventNotification( this, alarmFrame->id(), detail );
            }
        }
    }
    return( true );
}

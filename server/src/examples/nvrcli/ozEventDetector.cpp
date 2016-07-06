#include "ozEventDetector.h"

#include <base/ozMotionFrame.h>
#include <libgen/libgenTime.h>

/**
* @brief 
*
* @return 
*/
int EventDetector::run()
{
    if ( waitForProviders() )
    {
        while( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                /*for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    processFrame( *iter );
                }*/
				mFunction(1);
				//printf (">>>>>>>>>>>>>>>>>>>>> BAZOOKA GOT A FRAME ");
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

/**
* @brief 
*
* @param frame
*
* @return 
*/
bool EventDetector::processFrame( FramePtr frame )
{
    const MotionFrame *motionFrame = dynamic_cast<const MotionFrame *>(frame.get());
    //const VideoProvider *provider = dynamic_cast<const VideoProvider *>(frame->provider());

    AlarmState lastState = mState;
    uint64_t now = time64();

    if ( motionFrame->alarmed() )
    {
        mState = ALARM;
        mAlarmTime = now;
        if ( lastState == IDLE )
        {
            // Create new event
            mEventCount++;
            for ( FrameStore::const_iterator iter = mFrameStore.begin(); iter != mFrameStore.end(); iter++ )
            {
                const MotionFrame *frame = dynamic_cast<const MotionFrame *>( iter->get() );
                std::string path = stringtf( "%s/img-%s-%d-%ju.jpg", mLocation.c_str(), mName.c_str(), mEventCount, frame->id() );
                //Info( "PF:%d @ %dx%d", frame->pixelFormat(), frame->width(), frame->height() );
                Image image( frame->pixelFormat(), frame->width(), frame->height(), frame->buffer().data() );
                image.writeJpeg( path.c_str() );
            }
        }
    }
    else if ( lastState == ALARM )
    {
        mState = ALERT;
    }
    else if ( lastState == ALERT )
    {
        if ( frame->age( mAlarmTime ) > MAX_EVENT_TAIL_AGE )
            mState = IDLE;
    }

    if ( mState > IDLE )
    {
        std::string path;
        if ( mState == ALARM )
        {
            path = stringtf( "%s/img-%s-%d-%ju-A.jpg", mLocation.c_str(), mName.c_str(), mEventCount, motionFrame->id() );
        }
        else if ( mState == ALERT )
        {
            path = stringtf( "%s/img-%s-%d-%ju.jpg", mLocation.c_str(), mName.c_str(), mEventCount, motionFrame->id() );
        }
        Info( "PF:%d @ %dx%d", motionFrame->pixelFormat(), motionFrame->width(), motionFrame->height() );
        Image image( motionFrame->pixelFormat(), motionFrame->width(), motionFrame->height(), motionFrame->buffer().data() );
        image.writeJpeg( path.c_str() );
    }

    // Clear out old frames
    Debug( 5, "Got %lu frames in store", mFrameStore.size() );
    while( !mFrameStore.empty() )
    {
        FramePtr tempFrame = *(mFrameStore.begin());
        Debug( 5, "Frame %ju age %.2lf", tempFrame->id(), tempFrame->age() );
        if ( tempFrame->age() <= MAX_EVENT_HEAD_AGE )
            break;
        Debug( 5, "Deleting" );
        //delete tempFrame;
        mFrameStore.pop_front();
    }
    mFrameStore.push_back( frame );
    mFrameCount++;
    return( true );
}

#include "../base/oz.h"
#include "ozRateLimiter.h"

#include "../base/ozFeedFrame.h"

#include <sys/time.h>

/**
* @brief 
*
* @param name
* @param frameRate
* @param skip
*/
RateLimiter::RateLimiter( const std::string &name, FrameRate frameRate, bool skip ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mSkip( skip ),
    mFrameRate( frameRate )
{
}

/**
* @brief 
*
* @param frameRate
* @param provider
*/
RateLimiter::RateLimiter( FrameRate frameRate, VideoProvider &provider ) :
    VideoConsumer( cClass(), provider, gPolledVideoLink ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mSkip( false ),
    mFrameRate( frameRate )
{
}

/**
* @brief 
*
* @param frameRate
* @param skip
* @param provider
* @param link
*/
RateLimiter::RateLimiter( FrameRate frameRate, bool skip,  VideoProvider &provider, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mSkip( skip ),
    mFrameRate( frameRate )
{
}

/**
* @brief 
*/
RateLimiter::~RateLimiter()
{
}

/**
* @brief 
*
* @return 
*/
int RateLimiter::run()
{
    if ( waitForProviders() )
    {
        FeedLink providerLink = mProviders.begin()->second;

        double timeInterval = mFrameRate.interval();
        struct timeval now;
        gettimeofday( &now, NULL );
        long double currTime = now.tv_sec+((double)now.tv_usec/1000000.0);
        long double nextTime = currTime;
        while ( !mStop )
        {
            // Synchronise the output with the desired output frame rate
            while ( currTime < nextTime )
            {
                gettimeofday( &now, 0 );
                currTime = now.tv_sec+((double)now.tv_usec/1000000.0);
                usleep( INTERFRAME_TIMEOUT );
            }
            nextTime += timeInterval;

            FramePtr framePtr;
            if ( providerLink.isPolled() )
            {
                framePtr = provider()->pollFrame();
            }
            else if ( providerLink.isQueued() )
            {
                mQueueMutex.lock();
                if ( !mFrameQueue.empty() )
                {
                    if ( !mConsumers.empty() )
                    {
                        FrameQueue::iterator iter = mFrameQueue.begin();
                        framePtr = *iter;
                    }
                    if ( mSkip )
                        mFrameQueue.clear();
                    else
                        mFrameQueue.pop_front();
                }
                mQueueMutex.unlock();
            }
            if ( framePtr.get() )
            {
                distributeFrame( framePtr );
                mFrameCount++;
            }
            checkProviders();
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}

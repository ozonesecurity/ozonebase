#include "../base/zm.h"
#include "zmRateLimiter.h"

#include "../base/zmFeedFrame.h"

#include <sys/time.h>

RateLimiter::RateLimiter( const std::string &name, FrameRate frameRate, bool skip ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mSkip( skip ),
    mFrameRate( frameRate )
{
}

RateLimiter::RateLimiter( FrameRate frameRate, bool skip,  VideoProvider &provider, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mSkip( skip ),
    mFrameRate( frameRate )
{
}

RateLimiter::~RateLimiter()
{
}

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
                usleep( 1000 );
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

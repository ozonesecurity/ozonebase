#include "../zm.h"
#include "zmVideoFilter.h"

#include "../zmFeedFrame.h"
#include <sys/time.h>

DirectVideoFilter::DirectVideoFilter( AudioVideoProvider &provider ) :
    VideoConsumer( cClass(), provider, FeedLink( FEED_QUEUED, AudioVideoProvider::videoFramesOnly ) ),
    VideoProvider( cClass(), provider.name() )
{
}

DirectVideoFilter::~DirectVideoFilter()
{
}

// Don't locally queue just send the frames on
bool DirectVideoFilter::queueFrame( FramePtr framePtr, FeedProvider *provider )
{
    distributeFrame( framePtr );
    return( true );
}

QueuedVideoFilter::QueuedVideoFilter( AudioVideoProvider &provider ) :
    VideoConsumer( cClass(), provider, FeedLink( FEED_QUEUED, AudioVideoProvider::videoFramesOnly ) ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() )
{
}

QueuedVideoFilter::~QueuedVideoFilter()
{
}

int QueuedVideoFilter::run()
{
    if ( waitForProviders() )
    {
        while ( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                Debug( 3, "Got %zd frames on queue", mFrameQueue.size() );
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    distributeFrame( *iter );
                    mFrameCount++;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}

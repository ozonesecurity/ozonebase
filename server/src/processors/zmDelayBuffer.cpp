#include "../zm.h"
#include "zmDelayBuffer.h"

#include "../zmFeedFrame.h"

DelayBufferFilter::DelayBufferFilter( const std::string &name, double delay ) :
    GeneralProvider( cClass(), name ),
    Thread( identity() ),
    mDelay( delay )
{
}

DelayBufferFilter::~DelayBufferFilter()
{
}

int DelayBufferFilter::run()
{
	FrameQueue delayQueue;

    if ( waitForProviders() )
    {
        while ( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                Debug( 3, "Got %zd frames on incoming queue", mFrameQueue.size() );
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                	delayQueue.push_back( *iter );
                    mFrameCount++;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();

            if ( !delayQueue.empty() )
            {
                Debug( 3, "Got %zd frames on delayed queue", delayQueue.size() );
                int sentFrameCount = 0;
                for ( FrameQueue::iterator iter = delayQueue.begin(); iter != delayQueue.end(); iter++ )
                {
                	FramePtr framePtr = *iter;
                	if ( framePtr->age() < mDelay )
                		break;
                	distributeFrame( framePtr );
                	sentFrameCount++;
                }
                if ( sentFrameCount > 0 )
                {
            		delayQueue.erase( delayQueue.begin(), delayQueue.begin()+sentFrameCount );
            	}
            }

            checkProviders();

            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}

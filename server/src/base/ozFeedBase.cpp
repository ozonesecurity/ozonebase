#include "oz.h"
#include "ozFeedBase.h"
#include "ozFeedProvider.h"
#include "ozFeedConsumer.h"

FeedLink gQueuedFeedLink( FEED_QUEUED );
FeedLink gQueuedVideoLink( FEED_QUEUED, FeedProvider::videoFramesOnly );
FeedLink gQueuedAudioLink( FEED_QUEUED, FeedProvider::audioFramesOnly );
FeedLink gQueuedDataLink( FEED_QUEUED, FeedProvider::dataFramesOnly );

FeedLink gPolledFeedLink( FEED_POLLED );
FeedLink gPolledVideoLink( FEED_POLLED, FeedProvider::videoFramesOnly );
FeedLink gPolledAudioLink( FEED_POLLED, FeedProvider::audioFramesOnly );
FeedLink gPolledDataLink( FEED_POLLED, FeedProvider::dataFramesOnly );

/**
* @brief 
*
* @param frame
* @param consumer
*
* @return 
*/
bool FeedLink::compare( const FramePtr &frame, const FeedConsumer *consumer ) const
{
    for ( FeedComparatorList::const_iterator iter = mComparators.begin(); iter != mComparators.end(); iter++ )
        if ( !(*iter)( frame, consumer ) )
            return( false );
    return( true );
}

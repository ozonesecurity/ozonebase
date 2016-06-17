#include "oz.h"
#include "ozFeedBase.h"
#include "ozFeedConsumer.h"

FeedLink gQueuedFeedLink( FEED_QUEUED );
FeedLink gPolledFeedLink( FEED_POLLED );

/**
* @brief 
*
* @param frame
* @param consumer
*
* @return 
*/
bool FeedLink::compare( FramePtr frame, const FeedConsumer *consumer ) const
{
    for ( FeedComparatorList::const_iterator iter = mComparators.begin(); iter != mComparators.end(); iter++ )
        if ( !(*iter)( frame, consumer ) )
            return( false );
    return( true );
}

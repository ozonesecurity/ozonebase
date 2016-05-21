#include "zm.h"
#include "zmFeedBase.h"
#include "zmFeedConsumer.h"

FeedLink gQueuedFeedLink( FEED_QUEUED );
FeedLink gPolledFeedLink( FEED_POLLED );

bool FeedLink::compare( FramePtr frame, const FeedConsumer *consumer ) const
{
    for ( FeedComparatorList::const_iterator iter = mComparators.begin(); iter != mComparators.end(); iter++ )
        if ( !(*iter)( frame, consumer ) )
            return( false );
    return( true );
}

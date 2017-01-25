/** @addtogroup Processors */
/*@{*/


#ifndef OZ_VIDEO_FILTER_H
#define OZ_VIDEO_FILTER_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

///
/// Processor that filters out non-video frames and accesses them directly
///
class DirectVideoFilter : public VideoConsumer, public VideoProvider
{
CLASSID(DirectVideoFilter);

public:
    DirectVideoFilter( AudioVideoProvider &provider );
    ~DirectVideoFilter();

    bool queueFrame( const FramePtr &framePtr, FeedProvider *provider );

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    PixelFormat pixelFormat() const { return( videoProvider()->pixelFormat() ); }
};

///
/// Processor that filters out non-video frames and uses the normal queued interface
///
class QueuedVideoFilter : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(QueuedVideoFilter);

public:
    QueuedVideoFilter( AudioVideoProvider &provider );
    ~QueuedVideoFilter();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    PixelFormat pixelFormat() const { return( videoProvider()->pixelFormat() ); }

protected:
    int run();
};

#endif // OZ_VIDEO_FILTER_H


/*@}*/

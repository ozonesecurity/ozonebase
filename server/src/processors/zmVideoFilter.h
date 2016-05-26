#ifndef ZM_VIDEO_FILTER_H
#define ZM_VIDEO_FILTER_H

#include "../base/zmFeedBase.h"
#include "../base/zmFeedProvider.h"
#include "../base/zmFeedConsumer.h"

///
/// Processor that filters out non-video frames and accesses them directly
///
class DirectVideoFilter : public VideoConsumer, public VideoProvider
{
CLASSID(DirectVideoFilter);

public:
    DirectVideoFilter( AudioVideoProvider &provider );
    ~DirectVideoFilter();

    bool queueFrame( FramePtr framePtr, FeedProvider *provider );

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

#endif // ZM_VIDEO_FILTER_H

#ifndef ZM_FILTER_SWAPUV_H
#define ZM_FILTER_SWAPUV_H

#include "../base/zmFeedBase.h"
#include "../base/zmFeedProvider.h"
#include "../base/zmFeedConsumer.h"

#include "../libimg/libimgImage.h"

class VideoFrame;

///
/// Video processor that adds a timestamp annotation (derived from the frame timestamp) onto
/// a video frame feed.
///
class FilterSwapUV : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(FilterSwapUV);

public:
    FilterSwapUV( const std::string &name );
    FilterSwapUV( VideoProvider &provider, const FeedLink &link=gQueuedFeedLink );
    ~FilterSwapUV();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    PixelFormat pixelFormat() const { return( videoProvider()->pixelFormat() ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

protected:
    int run();
};

#endif // ZM_FILTER_SWAPUV_H

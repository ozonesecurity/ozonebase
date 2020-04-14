/** @addtogroup Processors */
/*@{*/


#ifndef OZ_FILTER_SWAPUV_H
#define OZ_FILTER_SWAPUV_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

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
    AVPixelFormat pixelFormat() const { return( videoProvider()->pixelFormat() ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

protected:
    int run();
};

#endif // OZ_FILTER_SWAPUV_H


/*@}*/

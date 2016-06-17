/** @addtogroup Processors */
/*@{*/


#ifndef ZM_IMAGE_TIMESTAMPER_H
#define ZM_IMAGE_TIMESTAMPER_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

#include "../libimg/libimgImage.h"

class VideoFrame;

///
/// Video processor that adds a timestamp annotation (derived from the frame timestamp) onto
/// a video frame feed.
///
class ImageTimestamper : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(ImageTimestamper);

private:
    std::string     mTimestampFormat;   ///< Timestamp format string
    Coord           mTimestampLocation; ///< Coordinates of the origina of the timestamp string.
                                        ///< The top left of the image is 0,0

public:
    ImageTimestamper( const std::string &name );
    ImageTimestamper( VideoProvider &provider, const FeedLink &link=gQueuedFeedLink );
    ~ImageTimestamper();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    PixelFormat pixelFormat() const { return( Image::getNativePixelFormat( videoProvider()->pixelFormat() ) ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

protected:
    bool timestampImage( Image *image, uint64_t timestamp );

    int run();
};

#endif // ZM_IMAGE_TIMESTAMPER_H


/*@}*/

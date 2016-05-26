#ifndef ZM_IMAGE_CONVERT_H
#define ZM_IMAGE_CONVERT_H

#include "../base/zmFeedBase.h"
#include "../base/zmFeedProvider.h"
#include "../base/zmFeedConsumer.h"

///
/// Processor that simply resizes the input frame to the specified size
///
class ImageConvert : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(ImageConvert);

private:
    PixelFormat     mPixelFormat;
    int             mWidth;
    int             mHeight;
    FrameRate       mFrameRate;

    struct SwsContext *mConvertContext;

public:
    ImageConvert( const std::string &name, PixelFormat pixelFormat, int width, int height );
    ImageConvert( PixelFormat pixelFormat, int width, int height, VideoProvider &provider, const FeedLink &link=gQueuedFeedLink );
    ~ImageConvert();

    PixelFormat pixelFormat() const { return( mPixelFormat ); }
    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

protected:
    int run();
};

#endif // ZM_IMAGE_CONVERT_H

/** @addtogroup Processors */
/*@{*/


#ifndef OZ_IMAGE_SCALE_H
#define OZ_IMAGE_SCALE_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

class Rational;

///
/// Processor that simply resizes the input frame by a defined factor
class ImageScale : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(ImageScale);

private:
    Rational    mScale;
    int         mWidth;
    int         mHeight;

    struct SwsContext *mScaleContext;

public:
    ImageScale( const std::string &name, const Rational &scale );
    ImageScale( const Rational &scale, VideoProvider &provider, const FeedLink &link=gQueuedFeedLink );
    ~ImageScale();

    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    PixelFormat pixelFormat() const { return( videoProvider()->pixelFormat() ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

    const Rational &scale() const { return( mScale ); }

protected:
    int run();
};

#endif // OZ_IMAGE_SCALE_H


/*@}*/

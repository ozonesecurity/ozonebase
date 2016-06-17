/** @addtogroup Providers */
/*@{*/


#ifndef ZM_SLAVE_VIDEO_H
#define ZM_SLAVE_VIDEO_H

#include "../base/ozFeedProvider.h"

class Image;

///
/// Simple provider that exposes methods to allow frames to be
/// relayed from other providers or tasks. Unthreaded.
///
class SlaveVideo : public VideoProvider
{
CLASSID(SlaveVideo);

protected:
    bool        mInitialised;
    uint16_t    mImageWidth;
    uint16_t    mImageHeight;
    PixelFormat mPixelFormat;
    FrameRate   mFrameRate;

public:
    SlaveVideo( const std::string &name );
    SlaveVideo( const std::string &name, uint16_t width, uint16_t height, PixelFormat pixelFormat, const FrameRate &frameRate );
    ~SlaveVideo();

public:
    void prepare( uint16_t width, uint16_t height, PixelFormat pixelFormat, const FrameRate &frameRate );
    void prepare( const FrameRate &frameRate );
    void relayImage( const Image &image );

    PixelFormat pixelFormat() const { return( mPixelFormat ); }
    uint16_t width() const { return( mImageWidth ); }
    uint16_t height() const { return( mImageHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }
};

#endif // ZM_SLAVE_VIDEO_H


/*@}*/

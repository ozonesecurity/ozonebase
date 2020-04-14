/** @addtogroup Providers */
/*@{*/


#ifndef OZ_SLAVE_VIDEO_H
#define OZ_SLAVE_VIDEO_H

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
    bool            mInitialised;
    uint16_t        mImageWidth;
    uint16_t        mImageHeight;
    AVPixelFormat   mAVPixelFormat;
    FrameRate       mFrameRate;

public:
    SlaveVideo( const std::string &name );
    SlaveVideo( const std::string &name, uint16_t width, uint16_t height, AVPixelFormat pixelFormat, const FrameRate &frameRate );
    ~SlaveVideo();

public:
    void prepare( uint16_t width, uint16_t height, AVPixelFormat pixelFormat, const FrameRate &frameRate );
    void prepare( const FrameRate &frameRate );
    void relayImage( const Image &image );

    AVPixelFormat pixelFormat() const { return( mAVPixelFormat ); }
    uint16_t width() const { return( mImageWidth ); }
    uint16_t height() const { return( mImageHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }
};

#endif // OZ_SLAVE_VIDEO_H


/*@}*/

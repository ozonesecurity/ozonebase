/** @addtogroup Encoders */
/*@{*/


#ifndef OZ_JPEG_ENCODER_H
#define OZ_JPEG_ENCODER_H

#include "../base/ozEncoder.h"
#include "../libgen/libgenThread.h"

///
/// Class for multi-part jpeg encoder thread. Used by the HTTPStream class to produce network
/// video streams.
///
class JpegEncoder : public Encoder, public Thread
{
CLASSID(JpegEncoder);

protected:
    uint16_t        mWidth;
    uint16_t        mHeight;
    FrameRate       mFrameRate;
    AVPixelFormat   mAVPixelFormat;
    uint8_t         mQuality;

public:
    static std::string getPoolKey( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint8_t quality );

public:
    JpegEncoder( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint8_t quality );
    ~JpegEncoder();

    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }
    AVPixelFormat pixelFormat() const { return( mAVPixelFormat ); }
    uint8_t quality() const { return( mQuality ); }

protected:
    void poolingExpired() { stop(); }
    int run();
};

#endif // OZ_JPEG_ENCODER_H


/*@}*/

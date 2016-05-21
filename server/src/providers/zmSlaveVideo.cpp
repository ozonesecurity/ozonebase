#include "../zm.h"
#include "zmSlaveVideo.h"

SlaveVideo::SlaveVideo( const std::string &id, uint16_t width, uint16_t height, PixelFormat pixelFormat, const FrameRate &frameRate ) :
    VideoProvider( cClass(), id ),
    mInitialised( true ),
    mImageWidth( width ),
    mImageHeight( height ),
    mPixelFormat( pixelFormat ),
    mFrameRate( frameRate )
{
}

SlaveVideo::SlaveVideo( const std::string &id ) :
    VideoProvider( cClass(), id ),
    mInitialised( false ),
    mImageWidth( 0 ),
    mImageHeight( 0 ),
    mPixelFormat( PIX_FMT_NONE )
{
}

SlaveVideo::~SlaveVideo()
{
}

void SlaveVideo::prepare( uint16_t width, uint16_t height, PixelFormat pixelFormat, const FrameRate &frameRate )
{
    mImageWidth = width;
    mImageHeight = height;
    mPixelFormat = pixelFormat;
    mFrameRate = frameRate;
    mInitialised = true;
}

void SlaveVideo::prepare( const FrameRate &frameRate )
{
    mFrameRate = frameRate;
    mInitialised = true;
}

void SlaveVideo::relayImage( const Image &image )
{
    if ( !mInitialised )
    {
        Fatal( "Unable to relay image when not initialised" );
    }
    else if ( mImageWidth == 0 || mImageHeight == 0 || mPixelFormat == PIX_FMT_NONE )  /// FIXME - Ugly, make your mind up
    {
        mImageWidth = image.width();
        mImageHeight = image.height();
        mPixelFormat = image.pixelFormat();
    }

    struct timeval now;
    gettimeofday( &now, NULL );
    uint64_t timestamp = ((uint64_t)now.tv_sec*1000000LL)+now.tv_usec;
    VideoFrame *frame = new VideoFrame( this, mFrameCount, timestamp, image.buffer() );
    distributeFrame( FramePtr( frame ) );
    mFrameCount++;
}

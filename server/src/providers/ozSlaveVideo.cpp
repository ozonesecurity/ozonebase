#include "../base/oz.h"
#include "ozSlaveVideo.h"

/**
* @brief 
*
* @param id
* @param width
* @param height
* @param pixelFormat
* @param frameRate
*/
SlaveVideo::SlaveVideo( const std::string &id, uint16_t width, uint16_t height, PixelFormat pixelFormat, const FrameRate &frameRate ) :
    VideoProvider( cClass(), id ),
    mInitialised( true ),
    mImageWidth( width ),
    mImageHeight( height ),
    mPixelFormat( pixelFormat ),
    mFrameRate( frameRate )
{
}

/**
* @brief 
*
* @param id
*/
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

/**
* @brief 
*
* @param width
* @param height
* @param pixelFormat
* @param frameRate
*/
void SlaveVideo::prepare( uint16_t width, uint16_t height, PixelFormat pixelFormat, const FrameRate &frameRate )
{
    mImageWidth = width;
    mImageHeight = height;
    mPixelFormat = pixelFormat;
    mFrameRate = frameRate;
    mInitialised = true;
}

/**
* @brief 
*
* @param frameRate
*/
void SlaveVideo::prepare( const FrameRate &frameRate )
{
    mFrameRate = frameRate;
    mInitialised = true;
}

/**
* @brief 
*
* @param image
*/
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

#include "../base/oz.h"
#include "ozMemoryInputV1.h"

#include "../base/ozFeedFrame.h"
#include <cmath>

/**
* @brief 
*
* @param id
* @param location
* @param memoryKey
* @param imageCount
* @param pixelFormat
* @param imageWidth
* @param imageHeight
*/
MemoryInputV1::MemoryInputV1( const std::string &id,
                              const std::string &location,
                              int memoryKey,
                              int imageCount,
                              PixelFormat pixelFormat,
                              uint16_t imageWidth,
                              uint16_t imageHeight
) :
    VideoProvider( cClass(), id ),
    MemoryIOV1( location, memoryKey, false ),
    Thread( identity() ),
    mImageCount( imageCount ),
    mPixelFormat( pixelFormat ),
    mImageWidth( imageWidth ),
    mImageHeight( imageHeight )
{
}

/**
* @brief 
*/
MemoryInputV1::~MemoryInputV1()
{
}

/**
* @brief 
*
* @return 
*/
int MemoryInputV1::run()
{
    SharedData sharedData;
    memset( &sharedData, 0, sizeof(sharedData) );

    while( !mStop )
    {
        Debug( 2,"Querying memory" );
        if ( queryMemory( &sharedData ) && sharedData.valid )
            break;
        Info( "Can't query shared memory" );
        usleep( 500000 );
    }
    //Info( "SHV: %d", sharedData.valid );
    //mImageCount = 40;
    //mPixelFormat = sharedData.imageFormat;
    //mPixelFormat = PIX_FMT_UYVY422;
    //mPixelFormat = PIX_FMT_YUYV422;
    //mPixelFormat = PIX_FMT_RGB24;
    //mFrameRate = 15;
    ////mImageWidth = sharedData.imageWidth;
    //mImageWidth = 720;
    ////mImageHeight = sharedData.imageHeight;
    //mImageHeight = 576;

    attachMemory( mImageCount, mPixelFormat, mImageWidth, mImageHeight );

    int index = mSharedData->last_write_index;
    int lastIndex = (index+1)%mImageCount;
    Snapshot *snap = &mImageBuffer[index];
    int timeDiff = tvDiffUsec( *mImageBuffer[lastIndex].timestamp, *snap->timestamp );
    mFrameRate = (int)std::lround((1000000.0*(mImageCount-1))/timeDiff);

    int lastWriteIndex = mImageCount;
    while( !mStop )
    {
        if ( mSharedData && mSharedData->valid )
        {
            if ( mSharedData->last_write_index < mImageCount && mSharedData->last_write_index != lastWriteIndex )
            {
                const FeedFrame *frame = loadFrame();
                //Info( "Sending frame %d", frame->id() );
                lastWriteIndex = mSharedData->last_write_index;
                distributeFrame( FramePtr( frame ) );
                //delete frame;
                mFrameCount++;
            }
        }
        usleep( INTERFRAME_TIMEOUT );
    }
    cleanup();
    return( !ended() );
}

/**
* @brief 
*
* @return 
*/
const FeedFrame *MemoryInputV1::loadFrame()
{
    int index = mSharedData->last_write_index;

    if ( index < 0 || index > mImageCount )
        Fatal( "Invalid shared memory index %d", index );

    Snapshot *snap = &mImageBuffer[index];
    Image snapImage( *(snap->image) );

    uint64_t timestamp = ((uint64_t)snap->timestamp->tv_sec*1000000LL)+snap->timestamp->tv_usec;
    VideoFrame *frame = new VideoFrame( this, mFrameCount, timestamp, snapImage.buffer() );

    return( frame );
}

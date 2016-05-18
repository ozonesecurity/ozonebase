#include "../zm.h"
#include "zmMemoryInputV1.h"

#include "../zmFeedFrame.h"

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

MemoryInputV1::~MemoryInputV1()
{
}

int MemoryInputV1::run()
{
    SharedData sharedData;
    memset( &sharedData, 0, sizeof(sharedData) );

    while( !mStop )
    {
        Info( "Querying memory" );
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
    int lastWriteIndex = 0;
    while( !mStop )
    {
        if ( !mSharedData || !mSharedData->valid )
        {
            stop();
            break;
        }
        if ( mSharedData->last_write_index != lastWriteIndex )
        {
            const FeedFrame *frame = loadFrame();
            //Info( "Sending frame %d", frame->id() );
            lastWriteIndex = mSharedData->last_write_index;
            distributeFrame( FramePtr( frame ) );
            //delete frame;
            mFrameCount++;
        }
        usleep( INTERFRAME_TIMEOUT );
    }
    cleanup();
    return( !ended() );
}

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

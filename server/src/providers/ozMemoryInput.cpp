#include "../base/oz.h"
#include "ozMemoryInput.h"

#include "../base/ozFeedFrame.h"

/**
* @brief 
*
* @param id
* @param location
* @param memoryKey
*/
MemoryInput::MemoryInput( const std::string &id, const std::string &location, int memoryKey ) :
    VideoProvider( cClass(), id ),
    MemoryIO( location, memoryKey, false ),
    Thread( identity() )
{
}

/**
* @brief 
*/
MemoryInput::~MemoryInput()
{
}

/**
* @brief 
*
* @return 
*/
int MemoryInput::run()
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
    Debug( 2,"SHV: %d", sharedData.valid );
    mImageFormat = sharedData.imageFormat;
    mImageWidth = sharedData.imageWidth;
    mImageHeight = sharedData.imageHeight;

    attachMemory( sharedData.imageCount, mImageFormat, mImageWidth, mImageHeight );
    uint64_t lastWriteTime = 0;
    while( !mStop )
    {
        if ( !mSharedData || !mSharedData->valid )
        {
            stop();
            break;
        }
        if ( mSharedData->lastWriteTime != lastWriteTime )
        {
            const FeedFrame *frame = loadFrame();
            lastWriteTime = mSharedData->lastWriteTime;
            distributeFrame( FramePtr( frame ) );
            //delete frame;
            mFrameCount++;
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
const FeedFrame *MemoryInput::loadFrame()
{
    int index = mSharedData->lastWriteIndex;

    if ( index < 0 || index > mSharedData->imageCount )
        Fatal( "Invalid shared memory index %d", index );

    Snapshot *snap = &mImageBuffer[index];
    Image snapImage( *(snap->image) );

    VideoFrame *frame = new VideoFrame( this, mFrameCount, *(snap->timestamp), snapImage.buffer() );

    return( frame );
}

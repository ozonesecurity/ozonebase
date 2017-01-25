#include "../base/oz.h"
#include "ozMemoryOutputV1.h"

#include "../base/ozFeedFrame.h"
#include "../base/ozFeedProvider.h"
#include "../libgen/libgenTime.h"

//#include <sys/types.h>
#include <sys/stat.h>
//#include <arpa/inet.h>
//#include <glob.h>

#if OZ_MEM_MAPPED
#include <sys/mman.h>
#include <fcntl.h>
#else // OZ_MEM_MAPPED
#include <sys/ipc.h>
#include <sys/shm.h>
#endif // OZ_MEM_MAPPED

/**
* @brief 
*
* @param name
* @param location
* @param memoryKey
*/
MemoryOutputV1::MemoryOutputV1( const std::string &name, const std::string &location, int memoryKey ) :
    VideoConsumer( cClass(), name ),
    MemoryIOV1( location, memoryKey, true ),
    Thread( identity() ),
    mImageCount( 0 )
{
}

/**
* @brief 
*/
MemoryOutputV1::~MemoryOutputV1()
{
}

/**
* @brief 
*
* @return 
*/
int MemoryOutputV1::run()
{
    if ( waitForProviders() )
    {
        PixelFormat pixelFormat = videoProvider()->pixelFormat();
        uint16_t width = videoProvider()->width();
        uint16_t height = videoProvider()->height();

        attachMemory( 40, pixelFormat, width, height );
        while( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    storeFrame( *iter );
                    //delete *iter;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    cleanup();
    return( 0 );
}

/**
* @brief 
*
* @param frame
*
* @return 
*/
bool MemoryOutputV1::storeFrame( const FramePtr &frame )
{
    const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame.get());
    Debug(2, "PF:%d @ %dx%d", videoFrame->pixelFormat(), videoFrame->width(), videoFrame->height() );

    Image image( videoFrame->pixelFormat(), videoFrame->width(), videoFrame->height(), frame->buffer().data() );

    int index = mImageCount%mImageBufferCount;
    mImageBuffer[index].timestamp->tv_sec = videoFrame->timestamp()/1000000LL;
    mImageBuffer[index].timestamp->tv_usec = videoFrame->timestamp()%1000000LL;
    mImageBuffer[index].image->copy( image );

    mSharedData->last_write_index = index;
    mSharedData->last_write_time = videoFrame->timestamp();

    mImageCount++;

    return( true );
}

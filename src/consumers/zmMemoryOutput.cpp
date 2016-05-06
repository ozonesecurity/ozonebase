#include "../zm.h"
#include "zmMemoryOutput.h"

#include "../zmFeedFrame.h"
#include "../zmFeedProvider.h"
#include "../libgen/libgenTime.h"

//#include <sys/types.h>
#include <sys/stat.h>
//#include <arpa/inet.h>
//#include <glob.h>

#if ZM_MEM_MAPPED
#include <sys/mman.h>
#include <fcntl.h>
#else // ZM_MEM_MAPPED
#include <sys/ipc.h>
#include <sys/shm.h>
#endif // ZM_MEM_MAPPED

MemoryOutput::MemoryOutput( const std::string &name, const std::string &location, int memoryKey ) :
    VideoConsumer( cClass(), name ),
    MemoryIO( location, memoryKey, true ),
    Thread( identity() ),
    mImageCount( 0 )
{
}

MemoryOutput::~MemoryOutput()
{
}

int MemoryOutput::run()
{
    if ( waitForProviders() )
    {
        PixelFormat pixelFormat = videoProvider()->pixelFormat();
        uint16_t width = videoProvider()->width();
        uint16_t height = videoProvider()->height();

        attachMemory( 10, pixelFormat, width, height );
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

bool MemoryOutput::storeFrame( FramePtr frame )
{
    const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame.get());
    Info( "PF:%d @ %dx%d", videoFrame->pixelFormat(), videoFrame->width(), videoFrame->height() );

    Image image( videoFrame->pixelFormat(), videoFrame->width(), videoFrame->height(), frame->buffer().data() );

    int index = mImageCount%mSharedData->imageCount;
    *(mImageBuffer[index].timestamp) = videoFrame->timestamp();
    mImageBuffer[index].image->copy( image );

    mSharedData->lastWriteIndex = index;
    mSharedData->lastWriteTime = videoFrame->timestamp();

    mImageCount++;

    return( true );
}

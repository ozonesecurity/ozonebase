#include "../base/oz.h"
#include "ozLocalFileOutput.h"

#include "../base/ozFeedFrame.h"
#include "../base/ozFeedProvider.h"

/**
* @brief 
*
* @return 
*/
int LocalFileOutput::run()
{
    if ( waitForProviders() )
    {
        while( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    writeFrame( iter->get() );
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
bool LocalFileOutput::writeFrame( const FeedFrame *frame )
{
    std::string path = stringtf( "%s/img-%s-%ju.jpg", mLocation.c_str(), mName.c_str(), frame->id() );
    Debug( 1,"Path: %s", path.c_str() );
    const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame);
    //const VideoProvider *provider = dynamic_cast<const VideoProvider *>(frame->provider());
    //Info( "PF:%d @ %dx%d", videoFrame->pixelFormat(), videoFrame->width(), videoFrame->height() );
    Image image( videoFrame->pixelFormat(), videoFrame->width(), videoFrame->height(), frame->buffer().data() );
    image.writeJpeg( path.c_str() );
    return( true );
}

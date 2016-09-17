#include "../base/oz.h"
#include "ozFilterSwapUV.h"

#include "../base/ozFeedFrame.h"
#include <sys/time.h>

/**
* @brief 
*
* @param name
*/
FilterSwapUV::FilterSwapUV( const std::string &name ) :
    VideoProvider( cClass(), name ),
    Thread( identity() )
{
}

/**
* @brief 
*
* @param provider
* @param link
*/
FilterSwapUV::FilterSwapUV( VideoProvider &provider, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() )
{
}

/**
* @brief 
*/
FilterSwapUV::~FilterSwapUV()
{
}

/**
* @brief 
*
* @return 
*/
int FilterSwapUV::run()
{
    if ( waitForProviders() )
    {
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat inputPixelFormat = videoProvider()->pixelFormat();
        ByteBuffer tempBuffer;
        int yChannelSize = inputWidth*inputHeight;
        int uvChannelSize = yChannelSize/4;

        if ( inputPixelFormat != PIX_FMT_YUV420P )
            Fatal( "Can't swap UV for pixel format %d", inputPixelFormat );

        while ( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                Debug( 3, "Got %zd frames on queue", mFrameQueue.size() );
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    //const VideoFrame *frame = dynamic_cast<const VideoFrame *>(iter->get());
                    //FramePtr framePtr( *iter );
                    const FeedFrame *frame = (*iter).get();

                    Debug(1, "%s / Provider: %s, Source: %s, Frame: %p (%ju / %.3f) - %lu", cname(), frame->provider()->cidentity(), frame->originator()->cidentity(), frame, frame->id(), frame->age(), frame->buffer().size() );

                    //Image image( inputPixelFormat, inputWidth, inputHeight, frame->buffer().data() );

                    tempBuffer.size( frame->buffer().size() );
                    memcpy( tempBuffer.data(), frame->buffer().data(), yChannelSize );
                    memcpy( tempBuffer.data()+yChannelSize, frame->buffer().data()+yChannelSize+uvChannelSize, uvChannelSize);
                    memcpy( tempBuffer.data()+yChannelSize+uvChannelSize, frame->buffer().data()+yChannelSize, uvChannelSize);
                    VideoFrame *videoFrame = new VideoFrame( this, *iter, mFrameCount, frame->timestamp(), tempBuffer );
                    distributeFrame( FramePtr( videoFrame ) );

                    //delete *iter;
                    mFrameCount++;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}

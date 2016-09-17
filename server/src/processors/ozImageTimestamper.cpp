#include "../base/oz.h"
#include "ozImageTimestamper.h"

#include "../base/ozFeedFrame.h"
#include <sys/time.h>

/**
* @brief 
*
* @param name
*/
ImageTimestamper::ImageTimestamper( const std::string &name ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mTimestampFormat( "%N - %y/%m/%d %H:%M:%S.%f" ),
    mTimestampLocation( 0, 0 )
{
}

/**
* @brief 
*
* @param provider
* @param link
*/
ImageTimestamper::ImageTimestamper( VideoProvider &provider, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mTimestampFormat( "%N - %y/%m/%d %H:%M:%S.%f" ),
    mTimestampLocation( 0, 0 )
{
}

/**
* @brief 
*/
ImageTimestamper::~ImageTimestamper()
{
}

/**
* @brief 
*
* @return 
*/
int ImageTimestamper::run()
{
    if ( waitForProviders() )
    {
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat inputPixelFormat = videoProvider()->pixelFormat();

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

                    Image image( inputPixelFormat, inputWidth, inputHeight, frame->buffer().data() );

                    if ( timestampImage( &image, frame->timestamp() ) )
                    {
                        VideoFrame *videoFrame = new VideoFrame( this, *iter, mFrameCount, frame->timestamp(), image.buffer() );
                        distributeFrame( FramePtr( videoFrame ) );
                    }
                    else
                    {
                        distributeFrame( *iter );
                    }

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

/**
* @brief 
*
* @param image
* @param timestamp
*
* @return 
*/
bool ImageTimestamper::timestampImage( Image *image, uint64_t timestamp )
{
    if ( mTimestampFormat[0] )
    {
        time_t timeSecs = timestamp/1000000;
        time_t timeUsecs = timestamp%1000000;

        // Expand the strftime macros first
        char labelTimeText[256];
        strftime( labelTimeText, sizeof(labelTimeText), mTimestampFormat.c_str(), localtime( &timeSecs ) );

        char labelText[1024];
        const char *pSrc = labelTimeText;
        char *pDest = labelText;
        while ( *pSrc && ((pDest-labelText) < sizeof(labelText)) )
        {
            if ( *pSrc == '%' )
            {
                bool foundMacro = false;
                switch ( *(pSrc+1) )
                {
                    case 'N' :
                        pDest += snprintf( pDest, sizeof(labelText)-(pDest-labelText), "%s", cname() );
                        foundMacro = true;
                        break;
                    /*case 'Q' :
                        pDest += snprintf( pDest, sizeof(labelText)-(pDest-labelText), "%s", mTriggerData->triggerShowtext );
                        foundMacro = true;
                        break;*/
                    case 'f' :
                        pDest += snprintf( pDest, sizeof(labelText)-(pDest-labelText), "%02ld", timeUsecs/10000 );
                        foundMacro = true;
                        break;
                }
                if ( foundMacro )
                {
                    pSrc += 2;
                    continue;
                }
            }
            *pDest++ = *pSrc++;
        }
        *pDest = '\0';
        image->annotate( labelText, mTimestampLocation );
        return( true );
    }
    return( false );
}

#include "../base/oz.h"
#include "ozImageConvert.h"

#include "../base/ozFeedFrame.h"

/**
* @brief 
*
* @param name
* @param pixelFormat
* @param width
* @param height
*/
ImageConvert::ImageConvert( const std::string &name, PixelFormat pixelFormat, int width, int height ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mPixelFormat( pixelFormat ),
    mWidth( width ),
    mHeight( height ),
    mConvertContext( NULL )
{
}

/**
* @brief 
*
* @param pixelFormat
* @param width
* @param height
* @param provider
* @param link
*/
ImageConvert::ImageConvert( PixelFormat pixelFormat, int width, int height, VideoProvider &provider, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mPixelFormat( pixelFormat ),
    mWidth( width ),
    mHeight( height ),
    mConvertContext( NULL )
{
}

/**
* @brief 
*/
ImageConvert::~ImageConvert()
{
}

/**
* @brief 
*
* @return 
*/
int ImageConvert::run()
{
    AVFrame *inputFrame = avcodec_alloc_frame();
    AVFrame *outputFrame = avcodec_alloc_frame();

    if ( waitForProviders() )
    {
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat inputPixelFormat = videoProvider()->pixelFormat();

        // Prepare for image format and size conversions
        mConvertContext = sws_getContext( inputWidth, inputHeight, inputPixelFormat, mWidth, mHeight, mPixelFormat, SWS_BICUBIC, NULL, NULL, NULL );
        if ( !mConvertContext )
            Fatal( "Unable to create conversion context" );

        Debug( 1, "Converting from %d x %d @ %d -> %d x %d @ %d", inputWidth, inputHeight, inputPixelFormat, mWidth, mHeight, mPixelFormat );
        Debug( 1,"%d bytes -> %d bytes",  avpicture_get_size( inputPixelFormat, inputWidth, inputHeight ), avpicture_get_size( mPixelFormat, mWidth, mHeight ) );

        // Make space for anything that is going to be output
        ByteBuffer outputBuffer;
        outputBuffer.size( avpicture_get_size( mPixelFormat, mWidth, mHeight ) );

        // To get offsets only
        avpicture_fill( (AVPicture *)outputFrame, outputBuffer.data(), mPixelFormat, mWidth, mHeight );

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

                    if ( mWidth != inputWidth || mHeight != inputHeight || mPixelFormat != inputPixelFormat )
                    {
                        // Requires conversion
                        Debug( 1, "%s / Provider: %s, Source: %s, Frame: %p (%ju / %.3f) - %lu", cname(), frame->provider()->cidentity(), frame->originator()->cidentity(), frame, frame->id(), frame->age(), frame->buffer().size() );

                        avpicture_fill( (AVPicture *)inputFrame, frame->buffer().data(), inputPixelFormat, inputWidth, inputHeight );

                        // Reformat the input frame to fit the desired output format
                        if ( sws_scale( mConvertContext, inputFrame->data, inputFrame->linesize, 0, inputHeight, outputFrame->data, outputFrame->linesize ) < 0 )
                            Fatal( "Unable to convert input frame (%d@%dx%d) to output frame (%d@%dx%d) at frame %ju", inputPixelFormat, inputWidth, inputHeight, mPixelFormat, mWidth, mHeight, mFrameCount );

                        VideoFrame *videoFrame = new VideoFrame( this, *iter, mFrameCount, frame->timestamp(), outputBuffer );
                        distributeFrame( FramePtr( videoFrame ) );
                    }
                    else
                    {
                        // Send it out 'as is'
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
        FeedProvider::cleanup();
        FeedConsumer::cleanup();
        sws_freeContext( mConvertContext );
    }
    av_free( outputFrame );
    av_free( inputFrame );
    return( !ended() );
}

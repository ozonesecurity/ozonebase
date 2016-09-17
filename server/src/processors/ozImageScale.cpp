#include "../base/oz.h"
#include "ozImageScale.h"

#include "../base/ozFeedFrame.h"
#include "../base/ozFfmpeg.h"

/**
* @brief 
*
* @param name
* @param pixelFormat
* @param width
* @param height
*/
ImageScale::ImageScale( const std::string &name, const Rational &scale ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mScale( scale ),
    mWidth( 0 ),
    mHeight( 0 ),
    mScaleContext( NULL )
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
ImageScale::ImageScale(const Rational &scale, VideoProvider &provider, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mScale( scale ),
    mWidth( 0 ),
    mHeight( 0 ),
    mScaleContext( NULL )
{
}

/**
* @brief 
*/
ImageScale::~ImageScale()
{
    if ( mScaleContext )
        sws_freeContext( mScaleContext );
    mScaleContext = NULL;
}

/**
* @brief 
*
* @return 
*/
int ImageScale::run()
{
    AVFrame *inputFrame = av_frame_alloc();
    AVFrame *outputFrame = av_frame_alloc();

    if ( waitForProviders() )
    {
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat pixelFormat = videoProvider()->pixelFormat();

        mWidth = inputWidth * mScale;
        mHeight = inputHeight * mScale;

        // Prepare for image format and size conversions
        mScaleContext = sws_getContext( inputWidth, inputHeight, pixelFormat, mWidth, mHeight, pixelFormat, SWS_BILINEAR, NULL, NULL, NULL );
        if ( !mScaleContext )
            Fatal( "Unable to create scale context" );

        Debug( 1,"Scaling from %d x %d -> %d x %d", inputWidth, inputHeight, mWidth, mHeight );
        Debug( 1,"%d bytes -> %d bytes",  avpicture_get_size( pixelFormat, inputWidth, inputHeight ), avpicture_get_size( pixelFormat, mWidth, mHeight ) );

        // Make space for anything that is going to be output
        ByteBuffer outputBuffer;
        outputBuffer.size( avpicture_get_size( pixelFormat, mWidth, mHeight ) );

        // To get offsets only
        avpicture_fill( (AVPicture *)outputFrame, outputBuffer.data(), pixelFormat, mWidth, mHeight );

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

                    if ( mWidth != inputWidth || mHeight != inputHeight )
                    {
                        // Requires conversion
                        Debug( 1,"%s / Provider: %s, Source: %s, Frame: %p (%ju / %.3f) - %lu", cname(), frame->provider()->cidentity(), frame->originator()->cidentity(), frame, frame->id(), frame->age(), frame->buffer().size() );

                        avpicture_fill( (AVPicture *)inputFrame, frame->buffer().data(), pixelFormat, inputWidth, inputHeight );

                        // Reformat the input frame to fit the desired output format
                        if ( sws_scale( mScaleContext, inputFrame->data, inputFrame->linesize, 0, inputHeight, outputFrame->data, outputFrame->linesize ) < 0 )
                            Fatal( "Unable to convert input frame (%dx%d) to output frame (%dx%d) at frame %ju", inputWidth, inputHeight, mWidth, mHeight, mFrameCount );

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
        sws_freeContext( mScaleContext );
        mScaleContext = NULL;
    }
    av_free( outputFrame );
    av_free( inputFrame );
    return( !ended() );
}

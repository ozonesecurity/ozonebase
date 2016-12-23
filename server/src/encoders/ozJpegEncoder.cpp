#include "../base/oz.h"
#include "ozJpegEncoder.h"

#include "../base/ozFeedFrame.h"
#include "../libgen/libgenBuffer.h"

#include <sys/time.h>

/**
* @brief 
*
* @param name
* @param width
* @param height
* @param frameRate
* @param quality
*
* @return 
*/
std::string JpegEncoder::getPoolKey( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint8_t quality )
{
    return( stringtf( "%s-jpeg-%dx%d@%d/%d(%d)", name.c_str(), width, height, frameRate.num, frameRate.den, quality ) );
}

/**
* @brief 
*
* @param name
* @param width
* @param height
* @param frameRate
* @param quality
*/
JpegEncoder::JpegEncoder( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint8_t quality ) :
    Encoder( cClass(), getPoolKey( name, width, height, frameRate, quality ) ),
    Thread( identity() ),
    mWidth( width ),
    mHeight( height ),
    mFrameRate( frameRate ),
    mPixelFormat( PIX_FMT_YUVJ420P ),
    mQuality( quality )
{
}

/**
* @brief 
*/
JpegEncoder::~JpegEncoder()
{
}

/**
* @brief 
*
* @return 
*/
int JpegEncoder::run()
{
    // Make sure ffmpeg is compiled with mpjpeg support
    AVCodec *codec = avcodec_find_encoder( CODEC_ID_MJPEG );
    if ( !codec )
        Fatal( "Can't find encoder codec" );

    mCodecContext = avcodec_alloc_context3( codec );

    mCodecContext->width = mWidth;
    mCodecContext->height = mHeight;
    //// Require frames per second for output
    mCodecContext->time_base = mFrameRate.timeBase();
    mCodecContext->pix_fmt = mPixelFormat;

    Debug( 2, "Time base = %d/%d", mCodecContext->time_base.num, mCodecContext->time_base.den );
    Debug( 2, "Pix fmt = %d", mCodecContext->pix_fmt );

    /* open it */
    if ( avcodec_open2( mCodecContext, codec, NULL ) < 0 )
        Fatal( "Unable to open encoder codec" );

    AVFrame *inputFrame = avcodec_alloc_frame();

    // Wait for video source to be ready
    if ( waitForProviders() )
    {
        // Find the source codec context
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat inputPixelFormat = videoProvider()->pixelFormat();

        // Make space for anything that is going to be output
        AVFrame *outputFrame = avcodec_alloc_frame();
        ByteBuffer outputBuffer;
        outputBuffer.size( avpicture_get_size( mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height ) );
        avpicture_fill( (AVPicture *)outputFrame, outputBuffer.data(), mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height );

        // Prepare for image format and size conversions
        struct SwsContext *convertContext = sws_getContext( inputWidth, inputHeight, inputPixelFormat, mCodecContext->width, mCodecContext->height, mCodecContext->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL );
        if ( !convertContext )
            Fatal( "Unable to create conversion context for encoder" );
        Debug(1, "Converting from %d x %d @ %d -> %d x %d @ %d", inputWidth, inputHeight, inputPixelFormat, mCodecContext->width, mCodecContext->height, mCodecContext->pix_fmt );
        Debug( 1,"%d bytes -> %d bytes",  avpicture_get_size( inputPixelFormat, inputWidth, inputHeight ), avpicture_get_size( mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height ) );

        int outSize = 0;
        double timeInterval = (double)mCodecContext->time_base.num/mCodecContext->time_base.den;
        struct timeval now;
        gettimeofday( &now, 0 );
        long double currTime = now.tv_sec+((double)now.tv_usec/1000000.0);
        long double nextTime = currTime;
        outputFrame->pts = 0;
        outputFrame->width = mCodecContext->width;
        outputFrame->height = mCodecContext->height;
        outputFrame->format = mCodecContext->pix_fmt;
        while ( !mStop )
        {
            // Synchronise the output with the desired output frame rate
            while ( currTime < nextTime )
            {
                gettimeofday( &now, 0 );
                currTime = now.tv_sec+((double)now.tv_usec/1000000.0);
                usleep( INTERFRAME_TIMEOUT );
            }
            nextTime += timeInterval;

            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                if ( !mConsumers.empty() )
                {
                    FrameQueue::iterator iter = mFrameQueue.begin();
                    const VideoFrame *inputVideoFrame = NULL;

                    while ( iter != mFrameQueue.end() )
                    {
                        const FeedFrame *frame = iter->get();
                        inputVideoFrame = dynamic_cast<const VideoFrame *>(frame);
                        if ( inputVideoFrame )
                            break;
                        iter++;
                    }

                    if ( inputVideoFrame )
                    {
                        Debug( 2,"PF:%d @ %dx%d", inputVideoFrame->pixelFormat(), inputVideoFrame->width(), inputVideoFrame->height() );

                        //encodeFrame( frame );
                        avpicture_fill( (AVPicture *)inputFrame, inputVideoFrame->buffer().data(), inputPixelFormat, inputWidth, inputHeight );

                        outputFrame->pts = inputVideoFrame->timestamp() * av_q2d(mCodecContext->time_base);
                        Debug( 5, "TS:%jd, PTS %jd", inputVideoFrame->timestamp(), outputFrame->pts );
                        //outputFrame->pts = av_rescale_q( inputVideo.timestamp, mCodecContext->time_base, sourceCodecContext->time_base );

                        // Reformat the input frame to fit the desired output format
                        Debug(2, "oFd:%p, oFls:%d", outputFrame->data, *(outputFrame->linesize) );
                        if ( sws_scale( convertContext, inputFrame->data, inputFrame->linesize, 0, inputHeight, outputFrame->data, outputFrame->linesize ) < 0 )
                            Fatal( "Unable to convert input frame (%d@%dx%d) to output frame (%d@%dx%d) at frame %ju", inputPixelFormat, inputWidth, inputHeight, mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height, mFrameCount );

                        // Encode the image
                        outSize = avcodec_encode_video( mCodecContext, outputBuffer.data(), outputBuffer.capacity(), outputFrame );
                        Debug( 5, "Encoding reports %d bytes", outSize );
                        if ( outSize > 0 )
                        {
                            outputBuffer.size( outSize );
                            Debug( 5, "PTS2 %jd", mCodecContext->coded_frame->pts );
                            Debug( 3, "Queueing frame %ju, outSize = %d", mFrameCount, outSize );
            /*
                            if ( mBaseTimestamp == 0 )
                            {
                                struct timeval now;
                                gettimeofday( &now, 0 );
                                mBaseTimestamp = ((uint64_t)now.tv_sec*1000000LL)+now.tv_usec;
                            }
                            uint64_t timestamp = mBaseTimestamp + ((packet.pts*mCodecContext->time_base.den)/mCodecContext->time_base.num);
            */
                            VideoFrame *outputVideoFrame = new VideoFrame( this, *iter, ++mFrameCount, mCodecContext->coded_frame->pts, outputBuffer );
                            distributeFrame( FramePtr( outputVideoFrame ) );
                        }
                    }
                }
                // Remove all other frames from the queue
                //for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                    //delete *iter;
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
        }
        av_free( convertContext );
    }
    cleanup();

    return( !ended() );
}

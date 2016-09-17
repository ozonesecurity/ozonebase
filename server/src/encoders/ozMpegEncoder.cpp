#include "../base/oz.h"
#include "ozMpegEncoder.h"

#include "../base/ozFeedFrame.h"
#include "../libgen/libgenTime.h"
#include <sys/time.h>

/**
* @brief 
*
* @param name
* @param width
* @param height
* @param frameRate
* @param bitRate
* @param quality
*
* @return 
*/
std::string MpegEncoder::getPoolKey( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality )
{
    return( stringtf( "%s-mpeg-%dx%d@%d/%d-%d(%d)", name.c_str(), width, height, frameRate.num, frameRate.den, bitRate, quality ) );
}

/**
* @brief 
*
* @param name
* @param width
* @param height
* @param frameRate
* @param bitRate
* @param quality
*/
MpegEncoder::MpegEncoder( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality ) :
    Encoder( cClass(), getPoolKey( name, width, height, frameRate, bitRate, quality ) ),
    Thread( identity() ),
    mWidth( width ),
    mHeight( height ),
    mFrameRate( frameRate ),
    mBitRate( bitRate ),
    mPixelFormat( PIX_FMT_YUV420P ),
    mQuality( quality ),
    mAvcProfile( 0 )
{
}

/**
* @brief 
*/
MpegEncoder::~MpegEncoder()
{
}

/**
* @brief 
*
* @param trackId
*
* @return 
*/
const std::string &MpegEncoder::sdpString( int trackId ) const
{
    if ( true || mSdpString.empty() )
    {
        char sdpFormatString[] =
            "m=video 0 RTP/AVP 96\r\n"
            "b=AS:64\r\n"
            "a=framerate:%.2f\r\n"
            "a=rtpmap:96 MP4V-ES/90000\r\n"    ///< FIXME - Check if this ever gets used
            //"a=fmtp:96 packetization-mode=1; profile-level-id=%02x00%02x; sprop-parameter-sets: %s\r\n"
            "a=fmtp:96 profile-level-id=%02x\r\n"
            "a=control:trackID=%d\r\n";
        std::string spsString;
        mSdpString = stringtf( sdpFormatString, mFrameRate.toDouble(), mAvcProfile, trackId );
    }
    return( mSdpString );
}

/**
* @brief 
*
* @return 
*/
int MpegEncoder::gopSize() const
{
    return( mCodecContext->gop_size );
}

#if 0
bool MpegEncoder::registerConsumer( FeedConsumer &consumer, const FeedLink &link )
{
    if ( Encoder::registerConsumer( consumer, link ) )
    {
        return( true );
    }
    return( false );
}

bool MpegEncoder::deregisterConsumer( FeedConsumer &consumer, bool reciprocate )
{
    return( Encoder::deregisterConsumer( consumer, reciprocate ) );
}
#endif

/**
* @brief 
*
* @return 
*/
int MpegEncoder::run()
{
    // TODO - This section needs to be rewritten to read the configuration from the values saved
    // for the streams via the web gui
    AVDictionary *opts = NULL;
    //avSetMPEGPreset( &opts, "fast" );
    //avSetMPEGProfile( &opts, "baseline" );
    avDictSet( &opts, "g", "24" );
    //avDictSet( &opts, "b", (int)mBitRate );
    //avDictSet( &opts, "bitrate", (int)mBitRate );
    //avDictSet( &opts, "crf", "24" );
    //avDictSet( &opts, "framerate", (double)mFrameRate );
    //avDictSet( &opts, "fps", (double)mFrameRate );
    //avDictSet( &opts, "r", (double)mFrameRate );
    //avDictSet( &opts, "timebase", "1/90000" );
    avDumpDict( opts );

    AVCodec *codec = avcodec_find_encoder( CODEC_ID_MPEG4 );
    if ( !codec )
        Fatal( "Can't find encoder codec" );

    mCodecContext = avcodec_alloc_context3( codec );

    mCodecContext->width = mWidth;
    mCodecContext->height = mHeight;
    //mCodecContext->time_base = TimeBase( 1, 90000 );
    mCodecContext->time_base = mFrameRate.timeBase();
    mCodecContext->bit_rate = mBitRate;
    mCodecContext->pix_fmt = mPixelFormat;

    mCodecContext->gop_size = 24;
    //mCodecContext->max_b_frames = 1;

    Debug( 2, "Time base = %d/%d", mCodecContext->time_base.num, mCodecContext->time_base.den );
    Debug( 2, "Pix fmt = %d", mCodecContext->pix_fmt );

    /* open it */
    if ( avcodec_open2( mCodecContext, codec, &opts ) < 0 )
        Fatal( "Unable to open encoder codec" );

    avDumpDict( opts );
    AVFrame *inputFrame = avcodec_alloc_frame();

    Debug(1, "%s:Waiting", cidentity() );
    if ( waitForProviders() )
    {
        Debug( 1,"%s:Waited", cidentity() );

        // Find the source codec context
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat inputPixelFormat = videoProvider()->pixelFormat();
        //FrameRate inputFrameRate = videoProvider()->frameRate();
        //Info( "CONVERT: %d-%dx%d -> %d-%dx%d",
            //inputPixelFormat, inputWidth, inputHeight,
            //mPixelFormat, mWidth, mHeight
        //);

        // Make space for anything that is going to be output
        AVFrame *outputFrame = avcodec_alloc_frame();
        ByteBuffer outputBuffer;
        outputBuffer.size( avpicture_get_size( mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height ) );
        avpicture_fill( (AVPicture *)outputFrame, outputBuffer.data(), mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height );

        // Prepare for image format and size conversions
        struct SwsContext *convertContext = sws_getContext( inputWidth, inputHeight, inputPixelFormat, mCodecContext->width, mCodecContext->height, mCodecContext->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL );
        if ( !convertContext )
            Fatal( "Unable to create conversion context for encoder" );

        int outSize = 0;
        uint64_t timeInterval = mFrameRate.intervalUsec();
        uint64_t currTime = time64();
        uint64_t nextTime = currTime;
        //outputFrame->pts = currTime;
        outputFrame->pts = 0;
        uint32_t ptsInterval = 90000/mFrameRate.toInt();
        //uint32_t ptsInterval = mFrameRate.intervalPTS( mCodecContext->time_base );
        while ( !mStop )
        {
            // Synchronise the output with the desired output frame rate
            while ( currTime < nextTime )
            {
                currTime = time64();
                usleep( INTERFRAME_TIMEOUT );
            }
            nextTime += timeInterval;

            FramePtr framePtr;
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                if ( !mConsumers.empty() )
                {
                    FrameQueue::iterator iter = mFrameQueue.begin();
                    framePtr = *iter;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();

            if ( framePtr.get() )
            {
                const FeedFrame *frame = framePtr.get();
                const VideoFrame *inputVideoFrame = dynamic_cast<const VideoFrame *>(frame);

                //Info( "Provider: %s, Source: %s, Frame: %p", inputVideoFrame->provider()->cidentity(), inputVideoFrame->originator()->cidentity(), inputVideoFrame );
                //Info( "PF:%d @ %dx%d", inputVideoFrame->pixelFormat(), inputVideoFrame->width(), inputVideoFrame->height() );

                avpicture_fill( (AVPicture *)inputFrame, inputVideoFrame->buffer().data(), inputPixelFormat, inputWidth, inputHeight );

                //outputFrame->pts = currTime;
                //Debug( 5, "PTS %jd", outputFrame->pts );
       
                // Reformat the input frame to fit the desired output format
                //Info( "SCALE: %d -> %d", int(inputFrame->data[0])%16, int(outputFrame->data[0])%16 );
                if ( sws_scale( convertContext, inputFrame->data, inputFrame->linesize, 0, inputHeight, outputFrame->data, outputFrame->linesize ) < 0 )
                    Fatal( "Unable to convert input frame (%d@%dx%d) to output frame (%d@%dx%d) at frame %ju", inputPixelFormat, inputWidth, inputHeight, mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height, mFrameCount );

                // Encode the image
                outSize = avcodec_encode_video( mCodecContext, outputBuffer.data(), outputBuffer.capacity(), outputFrame );
                Debug( 5, "Encoding reports %d bytes", outSize );
                if ( outSize > 0 )
                {
                    Debug( 2,"CPTS: %jd", mCodecContext->coded_frame->pts );
                    outputBuffer.size( outSize );
                    //Debug( 5, "PTS2 %lld", mCodecContext->coded_frame->pts );
                    //av_rescale_q(cocontext->coded_frame->pts, cocontext->time_base, videostm->time_base); 
                    VideoFrame *outputVideoFrame = new VideoFrame( this, ++mFrameCount, mCodecContext->coded_frame->pts, outputBuffer );
                    distributeFrame( FramePtr( outputVideoFrame ) );
                }
                outputFrame->pts += ptsInterval;   ///< FIXME - This can't be right, but it works...
            }
            checkProviders();
        }
        av_free( convertContext );
    }
    cleanup();

    return( !ended() );
}

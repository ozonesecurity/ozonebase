#include "../base/oz.h"
#include "ozH264Encoder.h"

#include "../base/ozFeedFrame.h"
#include "../base/ozFfmpeg.h"
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
std::string H264Encoder::getPoolKey( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality )
{
    return( stringtf( "%s-h264-%dx%d@%d/%d-%d(%d)", name.c_str(), width, height, frameRate.num, frameRate.den, bitRate, quality ) );
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
H264Encoder::H264Encoder( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality ) :
    Encoder( cClass(), getPoolKey( name, width, height, frameRate, bitRate, quality ) ),
    Thread( identity() ),
    mWidth( width ),
    mHeight( height ),
    mFrameRate( frameRate ),
    mBitRate( bitRate ),
    mPixelFormat( AV_PIX_FMT_YUV420P ),
    mQuality( quality ),
    mAvcLevel( 0 ),
    mAvcProfile( 0 )
{
}

/**
* @brief 
*/
H264Encoder::~H264Encoder()
{
}

/**
* @brief 
*
* @param trackId
*
* @return 
*/
const std::string &H264Encoder::sdpString( int trackId ) const
{
    if ( true || mSdpString.empty() )
    {
        char sdpFormatString[] =
            "m=video 0 RTP/AVP 96\r\n"
            "b=AS:64\r\n"
            "a=framerate:%.2f\r\n"
            "a=rtpmap:96 H264/90000\r\n"    ///< FIXME - Check if this ever gets used
            //"a=fmtp:96 packetization-mode=1; profile-level-id=%02x00%02x; sprop-parameter-sets: %s\r\n"
            "a=fmtp:96 packetization-mode=1; profile-level-id=%02x00%02x%s\r\n"
            "a=control:trackID=%d\r\n";
        std::string spsString;
        if ( !mSps.empty() )
            spsString = "; sprop-parameter-sets: "+base64Encode( mSps );
        mSdpString = stringtf( sdpFormatString, mFrameRate.toDouble(), mAvcProfile, mAvcLevel, spsString.c_str(), trackId );
    }
    return( mSdpString );
}

/**
* @brief 
*
* @return 
*/
int H264Encoder::gopSize() const
{
    return( mCodecContext->gop_size );
}

/**
* @brief 
*
* @param consumer
* @param link
*
* @return 
*/
bool H264Encoder::registerConsumer( FeedConsumer &consumer, const FeedLink &link )
{
    if ( Encoder::registerConsumer( consumer, link ) )
    {
        if ( !mInitialFrame.empty() )
        {
            //VideoFrame *outputVideoFrame = new VideoFrame( this, ++mFrameCount, mCodecContext->coded_frame->pts, mInitialFrame );
            VideoFrame *outputVideoFrame = new VideoFrame( this, ++mFrameCount, 0, mInitialFrame );
            consumer.queueFrame( FramePtr( outputVideoFrame ), this );
        }
        return( true );
    }
    return( false );
}

/**
* @brief 
*
* @param consumer
* @param reciprocate
*
* @return 
*/
bool H264Encoder::deregisterConsumer( FeedConsumer &consumer, bool reciprocate )
{
    return( Encoder::deregisterConsumer( consumer, reciprocate ) );
}

/**
* @brief 
*
* @return 
*/
int H264Encoder::run()
{
    // TODO - This section needs to be rewritten to read the configuration from the values saved
    // for the streams via the web gui
    AVDictionary *opts = NULL;
    //avSetH264Preset( &opts, "default" );
    //avSetH264Profile( &opts, "main" );
    //avDictSet( &opts, "level", "4.1" );
    avSetH264Preset( &opts, "ultrafast" );
    //avSetH264Profile( &opts, "baseline" );
    avDictSet( &opts, "level", "31" );
    avDictSet( &opts, "g", "24" );
    //avDictSet( &opts, "b", (int)mBitRate );
    //avDictSet( &opts, "bitrate", (int)mBitRate );
    //avDictSet( &opts, "crf", "24" );
    //avDictSet( &opts, "framerate", (double)mFrameRate );
    //avDictSet( &opts, "fps", (double)mFrameRate );
    //avDictSet( &opts, "r", (double)mFrameRate );
    //avDictSet( &opts, "timebase", "1/90000" );
    avDumpDict( opts );

    // Make sure ffmpeg is compiled with libx264 support
    AVCodec *codec = avcodec_find_encoder( AV_CODEC_ID_H264 );
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
        Debug(1, "%s:Waited", cidentity() );

        // Find the source codec context
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        AVPixelFormat inputPixelFormat = videoProvider()->pixelFormat();
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
                if ( mInitialFrame.empty() || !mConsumers.empty() )
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

                //Info( "Provider: %s, Source: %s, Frame: %d", inputVideoFrame->provider()->cidentity(), inputVideoFrame->originator()->cidentity(), inputVideoFrame->id() );
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
                    //Info( "CPTS: %jd", mCodecContext->coded_frame->pts );
                    outputBuffer.size( outSize );
                    //Debug( 5, "PTS2 %jd", mCodecContext->coded_frame->pts );
                    if ( mInitialFrame.empty() )
                    {
                        Debug( 3, "Looking for H.264 stream info" );
                        const uint8_t *startPos = outputBuffer.head();
                        startPos = h264StartCode( startPos, outputBuffer.tail() );
                        while ( startPos < outputBuffer.tail() )
                        {
                            while( !*(startPos++) )
                                ;
                            const uint8_t *nextStartPos = h264StartCode( startPos, outputBuffer.tail() );

                            int frameSize = nextStartPos-startPos;

                            unsigned char type = startPos[0] & 0x1F;
                            unsigned char nri = startPos[0] & 0x60;
                            Debug( 1, "Type %d, NRI %d (%02x)", type, nri>>5, startPos[0] );

                            if ( type == NAL_SEI )
                            {
                                // SEI
                                mSei.assign( startPos, frameSize );
                            }
                            else if ( type == NAL_SPS )
                            {
                                // SPS
                                Hexdump( 2, startPos, frameSize );
                                mSps.assign( startPos, frameSize );

                                if ( frameSize < 4 )
                                    Panic( "H.264 NAL type 7 frame too short (%d bytes) to extract level/profile", frameSize );
                                mAvcLevel = startPos[3];
                                mAvcProfile = startPos[1];
                                Debug( 2, "Got AVC level %d, profile %d", mAvcLevel, mAvcProfile );
                            }
                            else if ( type == NAL_PPS )
                            {
                                // PPS
                                Hexdump( 2, startPos, frameSize );
                                mPps.assign( startPos, frameSize );
                            }
                            startPos = nextStartPos;
                        }
                        mInitialFrame = outputBuffer;
                        //VideoFrame *outputVideoFrame = new VideoFrame( this, ++mFrameCount, mCodecContext->coded_frame->pts, mInitialFrame );
                    }
                    else
                    {
                        //av_rescale_q(cocontext->coded_frame->pts, cocontext->time_base, videostm->time_base); 
                        VideoFrame *outputVideoFrame = new VideoFrame( this, ++mFrameCount, mCodecContext->coded_frame->pts, outputBuffer );
                        distributeFrame( FramePtr( outputVideoFrame ) );
                    }
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

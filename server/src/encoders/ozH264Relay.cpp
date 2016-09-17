#include "../base/oz.h"
#include "ozH264Relay.h"

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
std::string H264Relay::getPoolKey( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality )
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
H264Relay::H264Relay( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality ) :
    Encoder( cClass(), getPoolKey( name, width, height, frameRate, bitRate, quality ) ),
    Thread( identity() ),
    mWidth( width ),
    mHeight( height ),
    mFrameRate( frameRate ),
    mBitRate( bitRate ),
    mPixelFormat( PIX_FMT_YUV420P ),
    mQuality( quality ),
    mAvcLevel( 31 ),
    mAvcProfile( 66 )
{
}

/**
* @brief 
*/
H264Relay::~H264Relay()
{
}

/**
* @brief 
*
* @param trackId
*
* @return 
*/
const std::string &H264Relay::sdpString( int trackId ) const
{
    if ( true || mSdpString.empty() )
    {
        sleep( 1 );
        char sdpFormatString[] =
            "m=video 0 RTP/AVP 96\r\n"
            "b=AS:64\r\n"
            "a=framerate:%.2f\r\n"
            "a=rtpmap:96 H264/90000\r\n"    ///< FIXME - Check if this ever gets used
            "a=fmtp:96 packetization-mode=1; profile-level-id=%02x00%02x%s\r\n"
            "a=control:trackID=1\r\n";
        std::string spsString;
        if ( !mSps.empty() )
            spsString = "; sprop-parameter-sets: "+base64Encode( mSps );
        mSdpString = stringtf( sdpFormatString, mFrameRate.toDouble(), mAvcProfile, mAvcLevel, spsString.c_str() );
    }
    return( mSdpString );
}

/**
* @brief 
*
* @return 
*/
int H264Relay::gopSize() const
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
bool H264Relay::registerConsumer( FeedConsumer &consumer, const FeedLink &link )
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
bool H264Relay::deregisterConsumer( FeedConsumer &consumer, bool reciprocate )
{
    return( Encoder::deregisterConsumer( consumer, reciprocate ) );
}

/**
* @brief 
*
* @return 
*/
int H264Relay::run()
{
    // Make sure ffmpeg is compiled with libx264 support
    AVCodec *codec = avcodec_find_encoder( CODEC_ID_H264 );
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

    //const unsigned char startCode[] = { 0x00, 0x00, 0x00, 0x01 };

    Debug( 1,"%s:Waiting", cidentity() );
    if ( waitForProviders() )
    {
        Debug( 1,"%s:Waited", cidentity() );

        //uint16_t inputWidth = videoProvider()->width();
        //uint16_t inputHeight = videoProvider()->height();
        //PixelFormat inputPixelFormat = videoProvider()->pixelFormat();
        //FrameRate inputFrameRate = videoProvider()->frameRate();

        uint64_t startTime = 0;
        while ( !mStop )
        {
            usleep( INTERFRAME_TIMEOUT );
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
                ByteBuffer frameBuffer = frame->buffer();
                //frameBuffer.consume( 4 ); // Remove length bytes
                //frameBuffer.insert( startCode, sizeof(startCode) ); // Add start code

                if ( mInitialFrame.empty() )
                {
                    Debug( 3, "Looking for H.264 stream info" );
                    const uint8_t *startPos = frameBuffer.head();
                    startPos = h264StartCode( startPos, frameBuffer.tail() );
                    while ( startPos < frameBuffer.tail() )
                    {
                        while( !*(startPos++) )
                            ;
                        const uint8_t *nextStartPos = h264StartCode( startPos, frameBuffer.tail() );

                        int frameSize = nextStartPos-startPos;

                        unsigned char type = startPos[0] & 0x1F;
                        unsigned char nri = startPos[0] & 0x60;
                        Debug( 1, "Type %d, NRI %d (%02x)", type, nri>>5, startPos[0] );

                        if ( type == NAL_SEI )
                        {
                            // SEI
                            Debug( 2, "Got SEI frame" );
                            mSei.assign( startPos, frameSize );
                        }
                        else if ( type == NAL_SPS )
                        {
                            // SPS
                            Debug( 2, "Got SPS frame" );
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
                            Debug( 2, "Got PPS frame" );
                            Hexdump( 2, startPos, frameSize );
                            mPps.assign( startPos, frameSize );
                        }
                        startPos = nextStartPos;
                    }
                    if ( mSps.empty() )
                        continue;
                    mInitialFrame = frameBuffer;
                    startTime = frame->timestamp();
                }
                Debug( 2, "Frame TS: %jd", frame->timestamp() );
                //VideoFrame *videoFrame = new VideoFrame( this, ++mFrameCount, mCodecContext->coded_frame->pts, frameBuffer );
                VideoFrame *videoFrame = new VideoFrame( this, frame->id(), frame->timestamp()-startTime, frameBuffer );
                Debug( 2, "Video Frame TS: %jd", videoFrame->timestamp() );
                distributeFrame( FramePtr( videoFrame ) );
            }
            checkProviders();
        }
    }
    cleanup();

    return( !ended() );
}

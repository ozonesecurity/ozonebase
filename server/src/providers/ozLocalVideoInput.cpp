#include "../base/oz.h"
#include "ozLocalVideoInput.h"

#include "../base/ozFfmpeg.h"
#include "../libgen/libgenDebug.h"

/**
* @brief 
*
* @param id
* @param source
*/
LocalVideoInput::LocalVideoInput( const std::string &id, const std::string &source ) :
    VideoProvider( cClass(), id ),
    Thread( identity() ),
    mSource( source ),
    mCodecContext( NULL ),
    mStream( NULL )
{
}

/**
* @brief 
*/
LocalVideoInput::~LocalVideoInput()
{
}

/**
* @brief 
*
* @return 
*/
int LocalVideoInput::run()
{
    AVInputFormat *inputFormat = av_find_input_format( "video4linux2" );
    if ( inputFormat == NULL)
        Fatal( "Can't load input format" );

#if 0
    AVProbeData probeData;
    probeData.filename = mSource.c_str();
    probeData.buf = new unsigned char[1024];
    probeData.buf_size = 1024;
    inputFormat = av_probe_input_format( &probeData, 0 );
    if ( inputFormat == NULL)
        Fatal( "Can't probe input format" );

    AVFormatParameters formatParameters ;
    memset( &formatParameters, 0, sizeof(formatParameters) );
    formatParameters.channels = 1;
    formatParameters.channel = 0;
    formatParameters.standard = "PAL";
    formatParameters.pix_fmt = AV_PIX_FMT_RGB24;
    //formatParameters.time_base.num = 1;
    //formatParameters.time_base.den = 10;
    formatParameters.width = 352;
    formatParameters.height = 288;
    //formatParameters.prealloced_context = 1;
#endif

    /* New API */
    AVDictionary *opts = NULL;
    av_dict_set( &opts, "standard", "PAL", 0 );
    av_dict_set( &opts, "video_size", "320x240", 0 );
    av_dict_set( &opts, "channel", "0", 0 );
    av_dict_set( &opts, "pixel_format", "rgb24", 0 );
    //av_dict_set( &opts, "framerate", "10", 0 );
    avDumpDict( opts );

    int avError = 0;
    AVFormatContext *formatContext = NULL;
    //if ( av_open_input_file( &formatContext, mSource.c_str(), inputFormat, 0, &formatParameters ) !=0 )
    if ( (avError = avformat_open_input( &formatContext, mSource.c_str(), inputFormat, &opts )) < 0 )
        Fatal( "Unable to open input %s due to: %s", mSource.c_str(), avStrError(avError) );

    avDumpDict( opts );
#if 0
    if ( av_open_input_stream( &formatContext, 0, mSource.c_str(), inputFormat, &formatParameters ) !=0 )
        Fatal( "Unable to open input %s due to: %s", mSource.c_str(), strerror(errno) );
#endif

    // Locate stream info from input
    if ( (avError = avformat_find_stream_info( formatContext, &opts )) < 0 )
        Fatal( "Unable to find stream info from %s due to: %s", mSource.c_str(), avStrError(avError) );
    
    if ( dbgLevel > DBG_INF )
        av_dump_format( formatContext, 0, mSource.c_str(), 0 );

    // Find first video stream present
    int videoStreamId = -1;
    for ( int i=0; i < formatContext->nb_streams; i++ )
    {
        if ( formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
        {
            videoStreamId = i;
            //set_context_opts( formatContext->streams[i]->codec, avcodec_opts[CODEC_TYPE_VIDEO], AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM );
            break;
        }
    }
    if ( videoStreamId == -1 )
        Fatal( "Unable to locate video stream in %s", mSource.c_str() );
    mStream = formatContext->streams[videoStreamId];
    mCodecContext = mStream->codec;

    // Try and get the codec from the codec context
    AVCodec *codec = NULL;
    if ( (codec = avcodec_find_decoder( mCodecContext->codec_id )) == NULL )
        Fatal( "Can't find codec for video stream from %s", mSource.c_str() );

    // Open the codec
    if ( avcodec_open2( mCodecContext, codec, &opts ) < 0 )
        Fatal( "Unable to open codec for video stream from %s", mSource.c_str() );

    //AVFrame *savedFrame = av_frame_alloc();

    // Allocate space for the native video frame
    AVFrame *frame = av_frame_alloc();

    // Determine required buffer size and allocate buffer
    int pictureSize = avpicture_get_size( mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height );
    
    ByteBuffer frameBuffer( pictureSize );
    
    //avpicture_fill( (AVPicture *)savedFrame, mLastFrame.mBuffer.data(), mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height);

    AVPacket packet;
    while( !mStop )
    {
        int frameComplete = false;
        while ( !frameComplete && (av_read_frame( formatContext, &packet ) >= 0) )
        {
            Debug( 5, "Got packet from stream %d", packet.stream_index );
            if ( packet.stream_index == videoStreamId )
            {
                frameComplete = false;
                if ( avcodec_decode_video2( mCodecContext, frame, &frameComplete, &packet ) < 0 )
                    Fatal( "Unable to decode frame at frame %ju", mFrameCount );

                Debug( 3, "Decoded video packet at frame %ju, pts %jd", mFrameCount, packet.pts );

                if ( frameComplete )
                {
                    Debug( 3, "Got frame %d, pts %jd (%.3f)", mCodecContext->frame_number, frame->pkt_pts, (((double)(packet.pts-mStream->start_time)*mStream->time_base.num)/mStream->time_base.den) );

                    avpicture_layout( (AVPicture *)frame, mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height, frameBuffer.data(), frameBuffer.capacity() );

                    uint64_t timestamp = packet.pts;
                    //Debug( 3, "%d: TS: %lld, TS1: %lld, TS2: %lld, TS3: %.3f", time( 0 ), timestamp, packet.pts, ((1000000LL*packet.pts*mStream->time_base.num)/mStream->time_base.den), (((double)packet.pts*mStream->time_base.num)/mStream->time_base.den) );
                    //Info( "%ld:TS: %lld, TS1: %lld, TS2: %lld, TS3: %.3f", time( 0 ), timestamp, packet.pts, ((1000000LL*packet.pts*mStream->time_base.num)/mStream->time_base.den), (((double)packet.pts*mStream->time_base.num)/mStream->time_base.den) );

                    VideoFrame *videoFrame = new VideoFrame( this, mCodecContext->frame_number, timestamp, frameBuffer );
                    distributeFrame( FramePtr( videoFrame ) );
                }
            }
            av_free_packet( &packet );
        }
        usleep( INTERFRAME_TIMEOUT );
    }
    cleanup();

    av_freep( &frame );
    if ( mCodecContext )
    {
       avcodec_close( mCodecContext );
       mCodecContext = NULL; // Freed by avformat_close_input
    }
    if ( formatContext )
    {
        avformat_close_input( &formatContext );
        formatContext = NULL;
        //av_free( formatContext );
    }
    return( !ended() );
}

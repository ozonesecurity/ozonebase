#include "../zm.h"
#include "zmRemoteVideoInput.h"

#include "../zmFfmpeg.h"
#include "../libgen/libgenDebug.h"

RemoteVideoInput::RemoteVideoInput( const std::string &name, const std::string &source, const std::string &format ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mSource( source ),
    mFormat( format ),
    mCodecContext( 0 ),
    mBaseTimestamp( 0 )
{
}

RemoteVideoInput::~RemoteVideoInput()
{
}

int RemoteVideoInput::run()
{
    if ( dbgLevel > DBG_INF )
        av_log_set_level( AV_LOG_DEBUG ); 
    else
        av_log_set_level( AV_LOG_QUIET ); 

    avcodec_register_all();
    avdevice_register_all();
    av_register_all();
    avformat_network_init();

    AVInputFormat *inputFormat = 0;
    if ( !mFormat.empty() )
    {
        inputFormat = av_find_input_format( mFormat.c_str() );
        if ( inputFormat == NULL)
            Fatal( "Can't load input format" );
    }

    while ( !mStop )
    {
        AVDictionary *dict = NULL;
        AVFormatContext *formatContext = NULL;
        if ( avformat_open_input( &formatContext, mSource.c_str(), inputFormat, &dict ) !=0 )
            Fatal( "Unable to open input %s due to: %s", mSource.c_str(), strerror(errno) );

        // Locate stream info from input
        if ( avformat_find_stream_info( formatContext, /*&dict*/NULL ) < 0 )
            Fatal( "Unable to find stream info from %s due to: %s", mSource.c_str(), strerror(errno) );
        
        //if ( dbgLevel > DBG_INF )
            //dump_format(formatContext, 0, mSource.c_str(), 0);

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

        mCodecContext = formatContext->streams[videoStreamId]->codec;

        // Try and get the codec from the codec context
        AVCodec *codec = NULL;
        if ( (codec = avcodec_find_decoder( mCodecContext->codec_id )) == NULL )
            Fatal( "Can't find codec for video stream from %s", mSource.c_str() );

        // Open the codec
        if ( avcodec_open2( mCodecContext, codec, /*&dict*/NULL ) < 0 )
            Fatal( "Unable to open codec for video stream from %s", mSource.c_str() );

        // Allocate space for the native video frame
        AVFrame *frame = avcodec_alloc_frame();

        // Determine required buffer size and allocate buffer
        int pictureSize = avpicture_get_size( mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height );
        
        ByteBuffer frameBuffer( pictureSize );
        
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
                        Debug( 3, "Got frame %ju", mFrameCount );

                        //Hexdump( DBG_INFO, frame->data[0], 128 );

                        //mFrameMutex.lock();
                        avpicture_layout( (AVPicture *)frame, mCodecContext->pix_fmt, mCodecContext->width, mCodecContext->height, frameBuffer.data(), frameBuffer.capacity() );

                        if ( mBaseTimestamp == 0 )
                        {
                            struct timeval now;
                            gettimeofday( &now, 0 );
                            mBaseTimestamp = ((uint64_t)now.tv_sec*1000000LL)+now.tv_usec;
                        }
                        uint64_t timestamp = mBaseTimestamp + ((packet.pts*mCodecContext->time_base.den)/mCodecContext->time_base.num);
                        VideoFrame *videoFrame = new VideoFrame( this, ++mFrameCount, timestamp, frameBuffer );
                        //mFrameMutex.unlock();
                        distributeFrame( FramePtr( videoFrame ) );
                        //Hexdump( DBG_INFO, mSavedBuffer.data(), 128 );
                    }
                }
                av_free_packet( &packet );
            }
            usleep( INTERFRAME_TIMEOUT );
        }
        av_freep( &frame );

        if ( mCodecContext )
        {
           avcodec_close( mCodecContext );
           mCodecContext = NULL; // Freed by av_close_input_file
        }
        if ( formatContext )
        {
            avformat_close_input( &formatContext );
            formatContext = NULL;
            //av_free( formatContext );
        }
        if ( !mStop )
        {
            Warning( "%s: Reconnecting", cidentity() );
            sleep( RECONNECT_TIMEOUT );
        }
    }
    cleanup();
    return( !ended() );
}

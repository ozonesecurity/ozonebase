#include "../base/oz.h"
#include "ozNetworkAVInput.h"

#include "../base/ozFfmpeg.h"
#include "../libgen/libgenDebug.h"

/**
* @brief 
*
* @param name
* @param source
* @param format
*/
NetworkAVInput::NetworkAVInput( const std::string &name, const std::string &source, const std::string &format, bool loop ) :
    AudioVideoProvider( cClass(), name ),
    Thread( identity() ),
    mSource( source ),
    mFormat( format ),
    mVideoCodecContext( NULL ),
    mAudioCodecContext( NULL ),
    mVideoStream( NULL ),
    mAudioStream( NULL ),
    mBaseTimestamp( 0 ),
	mLoop(loop)
{
}

/**
* @brief 
*/
NetworkAVInput::~NetworkAVInput()
{
}

/**
* @brief 
*
* @return 
*/
int NetworkAVInput::run()
{
    try
    {
        AVInputFormat *inputFormat = 0;
        if ( !mFormat.empty() )
        {
            inputFormat = av_find_input_format( mFormat.c_str() );
            if ( inputFormat == NULL)
                Fatal( "Can't load input format" );
        }

        //AVDictionary *dict = NULL;
        //int dictRet = av_dict_set(&dict,"xxx","yyy",0);
        //int dictRet = av_dict_set(&dict,"standard","ntsc",0);
        //dictRet = av_dict_set(&dict,"video_size","320x240",0);

        while ( !mStop )
        {
            struct timeval now;
            gettimeofday( &now, 0 );
            mBaseTimestamp = ((uint64_t)now.tv_sec*1000000LL)+now.tv_usec;

            AVFormatContext *formatContext = NULL;
            if ( avformat_open_input( &formatContext, mSource.c_str(), inputFormat, /*&dict*/NULL ) !=0 )
                Fatal( "Unable to open input %s due to: %s", mSource.c_str(), strerror(errno) );

            // Locate stream info from input
            if ( avformat_find_stream_info( formatContext, /*&dict*/NULL ) < 0 )
                Fatal( "Unable to find stream info from %s due to: %s", mSource.c_str(), strerror(errno) );
            
            if ( dbgLevel > DBG_INF )
                av_dump_format(formatContext, 0, mSource.c_str(), 0);

            // Find first video stream present
            int videoStreamId = -1;
            int audioStreamId = -1;
            for ( int i = 0; i < formatContext->nb_streams; i++ )
            {
                if ( formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
                {
                    videoStreamId = i;
                    //set_context_opts( formatContext->streams[i]->codec, avcodec_opts[CODEC_TYPE_VIDEO], AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM );
                }
                else if ( formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
                {
                    audioStreamId = i;
                }
            }

            mVideoStream = NULL;
            mAudioStream = NULL;

            if ( videoStreamId >= 0 )
            {
                mVideoStream = formatContext->streams[videoStreamId];
                mVideoCodecContext = mVideoStream->codec;
                mHasVideo = true;
            }
            else
                Warning( "Unable to locate video stream in %s", mSource.c_str() );

            if ( audioStreamId > 0 )
            {
                mAudioStream = formatContext->streams[audioStreamId];
                mAudioCodecContext = mAudioStream->codec;
                mHasAudio = true;
            }
            else
                Warning( "Unable to locate audio stream in %s", mSource.c_str() );

            // Try and get the codec from the codec context
            AVCodec *videoCodec = NULL;
            if ( mHasVideo )
            {
                if ( (videoCodec = avcodec_find_decoder( mVideoCodecContext->codec_id )) == NULL )
                    Fatal( "Can't find codec for video stream from %s", mSource.c_str() );

                Debug( 2, "Video Info - Pixel Format: %s, Size %dx%d, Frame Rate: %d/%d (%.2lf), GOP Size: %d, Bit Rate: %d, Frame Size: %d",
                    av_get_pix_fmt_name(mVideoCodecContext->pix_fmt),
                    mVideoCodecContext->width,
                    mVideoCodecContext->height,
                    mVideoStream->r_frame_rate.num,
                    mVideoStream->r_frame_rate.den,
                    (double)mVideoStream->r_frame_rate.num/mVideoStream->r_frame_rate.den,
                    mVideoCodecContext->gop_size,
                    mVideoCodecContext->bit_rate,
                    mVideoCodecContext->frame_size
                );

                // Open the codec
                if ( avcodec_open2( mVideoCodecContext, videoCodec, /*&dict*/NULL ) < 0 )
                    Fatal( "Unable to open codec for video stream from %s", mSource.c_str() );
                Debug( 1, "Opened video codec %s (%s)", videoCodec->name, videoCodec->long_name );
            }

            AVCodec *audioCodec = NULL;
            if ( mHasAudio )
            {
                if ( (audioCodec = avcodec_find_decoder( mAudioCodecContext->codec_id )) == NULL )
                    Fatal( "Can't find codec for video stream from %s", mSource.c_str() );

                Debug( 2, "Audio Info - Sample Format: %s, Bit Rate: %d, Sample Rate: %d, Channels: %d, Frame Size: %d",
                    av_get_sample_fmt_name(mAudioCodecContext->sample_fmt),
                    mAudioCodecContext->bit_rate,
                    mAudioCodecContext->sample_rate,
                    mAudioCodecContext->channels,
                    mAudioCodecContext->frame_size
                );

                // some formats want stream headers to be separate
                if ( avcodec_open2( mAudioCodecContext, audioCodec, /*&dict*/NULL ) < 0 )
                    Fatal( "Unable to open codec for video stream from %s", mSource.c_str() );
                Debug( 1, "Opened audio codec %s (%s)", audioCodec->name, audioCodec->long_name );
            }

            //if (oc->oformat->flags & AVFMT_GLOBALHEADER)
                //c->flags |= CODEC_FLAG_GLOBAL_HEADER;

            // Allocate space for the native video frame
            AVFrame *avVideoFrame = NULL;
            AVFrame *avAudioFrame = NULL;

            // Determine required buffer size and allocate buffer
            int videoFrameSize = 0;
            int audioFrameSize = 0;
         
            // Allocate space for the native video frame
            if ( mHasVideo )
            {
                avVideoFrame = avcodec_alloc_frame();
                videoFrameSize = avpicture_get_size( mVideoCodecContext->pix_fmt, mVideoCodecContext->width, mVideoCodecContext->height );
            }

            if ( mHasAudio )
            {
                avAudioFrame = avcodec_alloc_frame();
                audioFrameSize = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;
            }

            ByteBuffer videoFrameBuffer( videoFrameSize );
            ByteBuffer audioFrameBuffer( audioFrameSize );
            
            AVPacket packet;
            av_init_packet(&packet);
            while( !mStop )
            {
                int videoFrameComplete = false;
                int audioFrameComplete = false;
                //while ( !frameComplete && (av_read_frame( formatContext, &packet ) >= 0) )
                int readLeft = av_read_frame (formatContext, &packet);
                while ( !mStop && readLeft  >=0 ) 
                {
                    Debug( 5, "Got packet from stream %d", packet.stream_index );
                    if ( mBaseTimestamp == 0 )
                    {
                        struct timeval now;
                        gettimeofday( &now, 0 );
                        mBaseTimestamp = ((uint64_t)now.tv_sec*1000000LL)+now.tv_usec;
                    }
                    if ( mHasVideo && (packet.stream_index == videoStreamId) )
                    {
                        videoFrameComplete = false;
                        if ( avcodec_decode_video2( mVideoCodecContext, avVideoFrame, &videoFrameComplete, &packet ) < 0 )
                            Fatal( "%s: Unable to decode video frame at frame %ju", cidentity(), VideoProvider::mFrameCount );

                        int64_t pts;
                        if( packet.dts != AV_NOPTS_VALUE) {
                            pts = av_frame_get_best_effort_timestamp(avVideoFrame);
                        } else {
                            pts = 0;
                        }

                        Debug( 3, "%s: Decoded video packet at frame %d, pts %jd", cidentity(), mVideoCodecContext->frame_number, pts );

                        if ( videoFrameComplete )
                        {
                            double timeOffset = pts * av_q2d(mVideoStream->time_base);
                            Debug( 3, "%s: Got video frame %d, pts %jd (%.3f)", cidentity(), mVideoCodecContext->frame_number, pts, timeOffset );

                            avpicture_layout( (AVPicture *)avVideoFrame, mVideoCodecContext->pix_fmt, mVideoCodecContext->width, mVideoCodecContext->height, videoFrameBuffer.data(), videoFrameBuffer.capacity() );

                            uint64_t timestamp = mBaseTimestamp + (1000000.0L*timeOffset);
                            //Info( "%ld: TS: %jd, TS1: %jd, TS2: %jd, TS3: %.3f", time( 0 ), timestamp, packet.pts, (uint64_t)(1000000.0L*timeOffset), timeOffset );
         
                            VideoFrame *videoFrame = new VideoFrame( this, mVideoCodecContext->frame_number, timestamp, videoFrameBuffer );
                            distributeFrame( FramePtr( videoFrame ) );
                        }
                    }
                    else if ( mHasAudio && packet.stream_index == audioStreamId )
                    {
                        audioFrameComplete = false;
                        if ( avcodec_decode_audio4( mAudioCodecContext, avAudioFrame, &audioFrameComplete, &packet ) < 0 )
                            Fatal( "Unable to decode audio frame at frame %ju", AudioProvider::mFrameCount );

                        Debug( 3, "Decoded audio packet at frame %d, pts %jd", mAudioCodecContext->frame_number, packet.pts );

                        if ( audioFrameComplete )
                        {
                            int64_t pts;
                            if( packet.dts != AV_NOPTS_VALUE) {
                                pts = av_frame_get_best_effort_timestamp(avVideoFrame);
                            } else {
                                pts = 0;
                            }

                            double timeOffset = pts * av_q2d(mAudioStream->time_base);

                            audioFrameSize = av_samples_get_buffer_size( avAudioFrame->linesize, mAudioCodecContext->channels, avAudioFrame->nb_samples, mAudioCodecContext->sample_fmt, 1 ) + FF_INPUT_BUFFER_PADDING_SIZE;
                            audioFrameBuffer.size( audioFrameSize );

                            Debug( 3, "Got audio frame %d, pts %jd (%.3f)", mAudioCodecContext->frame_number, pts, timeOffset );

                            uint64_t timestamp = mBaseTimestamp + (1000000.0L*timeOffset);
                            //Debug( 3, "%d: TS: %jd, TS1: %jd, TS2: %jd, TS3: %.3f", time( 0 ), timestamp, packet.pts, ((1000000LL*packet.pts*mAudioStream->time_base.num)/mAudioStream->time_base.den), (((double)packet.pts*mAudioStream->time_base.num)/mAudioStream->time_base.den) );

                            AudioFrame *audioFrame = new AudioFrame( this, mAudioCodecContext->frame_number, timestamp, audioFrameBuffer, avAudioFrame->nb_samples );
                            distributeFrame( FramePtr( audioFrame ) );
                        }
                    }
                    av_free_packet( &packet );
                    readLeft = av_read_frame (formatContext, &packet);
                    if ((readLeft  <0)  && mLoop) 
                    {
                        Debug (2,"Looping video...");
                        if (av_seek_frame(formatContext, -1, 0, AVSEEK_FLAG_ANY) >=0)
                        {
                            readLeft = av_read_frame (formatContext, &packet);
                        }
                    }
                }
                usleep( INTERFRAME_TIMEOUT );
            }
            if ( mHasVideo )
            {
                av_freep( &avVideoFrame );
                mVideoStream = NULL;
                if ( mVideoCodecContext )
                {
                    avcodec_close( mVideoCodecContext );
                    mVideoCodecContext = NULL; // Freed by avformat_close_input
                }
            }
            if ( mHasAudio )
            {
                av_freep( &avAudioFrame );
                mAudioStream = NULL;
                if ( mAudioCodecContext )
                {
                    avcodec_close( mAudioCodecContext );
                    mAudioCodecContext = NULL; // Freed by avformat_close_input
                }
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
    } // try
    catch (std::exception &e)
    {
        Error (e.what());
        cleanup();
        setError();
        return (error());
           
    }
    
}

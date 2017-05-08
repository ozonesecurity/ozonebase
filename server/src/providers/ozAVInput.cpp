#include "../base/oz.h"
#include "ozAVInput.h"

#include "../base/ozFfmpeg.h"
#include "../libgen/libgenDebug.h"

AVInput::AVInput( const std::string &name, const std::string &source, const Options &options ) :
    AudioVideoProvider( cClass(), name ),
    Thread( identity() ),
    mSource( source ),
    mOptions( options ),
    mVideoCodecContext( NULL ),
    mAudioCodecContext( NULL ),
    mVideoStream( NULL ),
    mAudioStream( NULL ),
    mVideoStreamId( -1 ),
    mAudioStreamId( -1 ),
    mVideoFrame( NULL ),
    mAudioFrame( NULL ),
    mBaseTimestamp( 0 ),
    mLastTimestamp( 0 )
{
    mRealtime = mOptions.get( "realtime", false );
}

/**
* @brief 
*/
AVInput::~AVInput()
{
}

int AVInput::decodePacket( AVPacket &packet, int &frameComplete )
{
    int decodedBytes = packet.size;

    frameComplete = false;
    if ( mHasVideo && (packet.stream_index == mVideoStreamId) )
    {
        int result = avcodec_decode_video2( mVideoCodecContext, mVideoFrame, &frameComplete, &packet );
        if ( result < 0 )
        {
            Error( "%s: Unable to decode video frame at frame %ju", cidentity(), VideoProvider::mFrameCount );
            return( -1 );
        }

        if ( frameComplete )
        {
            if ( packet.dts != AV_NOPTS_VALUE )
            {
                int64_t pts = av_frame_get_best_effort_timestamp(mVideoFrame);

                Debug( 3, "%s: Decoded video packet at frame %d, pts %jd", cidentity(), mVideoCodecContext->frame_number, pts );

                double timeOffset = pts * av_q2d(mVideoStream->time_base);
                Debug( 3, "%s: Got video frame %d, pts %jd (%.3f)", cidentity(), mVideoCodecContext->frame_number, pts, timeOffset );

                avpicture_layout( (AVPicture *)mVideoFrame, mVideoCodecContext->pix_fmt, mVideoCodecContext->width, mVideoCodecContext->height, mVideoFrameBuffer.data(), mVideoFrameBuffer.capacity() );

                mLastTimestamp = mBaseTimestamp + (1000000.0L*timeOffset);
                Debug( 1,"%ld: TS: %jd, TS1: %jd, TS2: %jd, TS3: %.3f", time( 0 ), mLastTimestamp, packet.pts, (uint64_t)(1000000.0L*timeOffset), timeOffset );

                if ( mRealtime )
                {
                    uint64_t mNowTimestamp = time64();
                    //Info( "n:%jd, t:%jd, D:%jd", mNowTimestamp, mLastTimestamp, (mLastTimestamp - mNowTimestamp) );
                    if ( mLastTimestamp > mNowTimestamp )
                        usleep( mLastTimestamp - mNowTimestamp );
                }

                VideoFrame *videoFrame = new VideoFrame( this, mVideoCodecContext->frame_number, mLastTimestamp, mVideoFrameBuffer );
                distributeFrame( FramePtr( videoFrame ) );
            }
        }
    }
    else if ( mHasAudio && packet.stream_index == mAudioStreamId )
    {
        int result = avcodec_decode_audio4( mAudioCodecContext, mAudioFrame, &frameComplete, &packet );
        if ( result < 0 )
        {
            Error( "Unable to decode audio frame at frame %ju", AudioProvider::mFrameCount );
            return( result );
        }
        decodedBytes = FFMIN(result, packet.size);

        if ( frameComplete )
        {
            if ( packet.dts != AV_NOPTS_VALUE )
            {
                int64_t pts = av_frame_get_best_effort_timestamp(mAudioFrame);

                Debug( 3, "Decoded audio packet at frame %d, pts %jd", mAudioCodecContext->frame_number, pts );

                double timeOffset = pts * av_q2d(mAudioStream->time_base);

                int audioFrameSize = av_samples_get_buffer_size( mAudioFrame->linesize, mAudioCodecContext->channels, mAudioFrame->nb_samples, mAudioCodecContext->sample_fmt, 1 ) + FF_INPUT_BUFFER_PADDING_SIZE;
                mAudioFrameBuffer.size( audioFrameSize );

                Debug( 3, "Got audio frame %d, pts %jd (%.3f)", mAudioCodecContext->frame_number, pts, timeOffset );

                mLastTimestamp = mBaseTimestamp + (1000000.0L*timeOffset);
                //Debug( 3, "%d: TS: %jd, TS1: %jd, TS2: %jd, TS3: %.3f", time( 0 ), mLastTimestamp, packet.pts, ((1000000LL*packet.pts*mAudioStream->time_base.num)/mAudioStream->time_base.den), (((double)packet.pts*mAudioStream->time_base.num)/mAudioStream->time_base.den) );

                if ( mRealtime )
                {
                    uint64_t mNowTimestamp = time64();
                    //Info( "n:%jd, t:%jd, D:%jd", mNowTimestamp, mLastTimestamp, (mLastTimestamp - mNowTimestamp) );
                    if ( mLastTimestamp > mNowTimestamp )
                        usleep( mLastTimestamp - mNowTimestamp );
                }

                AudioFrame *audioFrame = new AudioFrame( this, mAudioCodecContext->frame_number, mLastTimestamp, mAudioFrameBuffer, mAudioFrame->nb_samples );
                distributeFrame( FramePtr( audioFrame ) );
            }
        }
    }
    return( decodedBytes );
}

/**
* @brief 
*
* @return 
*/
int AVInput::run()
{
    try
    {
        AVInputFormat *inputFormat = 0;
        const std::string format = mOptions.get( "format", "" );
        const std::string framerate = mOptions.get( "framerate","" );
        const std::string videosize = mOptions.get( "videosize","" );
        Info ("framerate is:%s",framerate.c_str());
      
        if ( !format.empty() )
        {
            inputFormat = av_find_input_format( format.c_str() );
            if ( inputFormat == NULL)
                Fatal( "Can't load input format" );
        }

        bool loop = mOptions.get( "loop", false );

        AVDictionary *dict = NULL;

        if (!framerate.empty())
        {
                av_dict_set(&dict, "framerate", framerate.c_str(), 0);
        }

        if (!videosize.empty())
        {
                av_dict_set(&dict, "video_size", videosize.c_str(), 0);

        }

        //int dictRet = av_dict_set(&dict,"xxx","yyy",0);
        //int dictRet = av_dict_set(&dict,"standard","ntsc",0);
        //dictRet = av_dict_set(&dict,"video_size","320x240",0);

        while ( !mStop )
        {
            //Info ("AVInput mStop is:%d",mStop);
            AVFormatContext *formatContext = NULL;
            if ( avformat_open_input( &formatContext, mSource.c_str(), inputFormat, &dict ) !=0 )
                Fatal( "Unable to open input %s due to: %s", mSource.c_str(), strerror(errno) );

            // Locate stream info from input
            if ( avformat_find_stream_info( formatContext, /*&dict*/NULL ) < 0 )
                Fatal( "Unable to find stream info from %s due to: %s", mSource.c_str(), strerror(errno) );

            if ( dbgLevel > DBG_INF )
                av_dump_format(formatContext, 0, mSource.c_str(), 0);

            // Find first video stream present
            mVideoStreamId = -1;
            mAudioStreamId = -1;
            for ( int i = 0; i < formatContext->nb_streams; i++ )
            {
                if ( formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
                {
                    mVideoStreamId = i;
                    //set_context_opts( formatContext->streams[i]->codec, avcodec_opts[CODEC_TYPE_VIDEO], AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM );
                }
                else if ( formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
                {
                    mAudioStreamId = i;
                }
            }

            mVideoStream = NULL;
            mAudioStream = NULL;

            if ( mVideoStreamId >= 0 )
            {
                mVideoStream = formatContext->streams[mVideoStreamId];
                mVideoCodecContext = mVideoStream->codec;
                mHasVideo = true;
            }
            else
                Warning( "Unable to locate video stream in %s", mSource.c_str() );

            if ( mAudioStreamId > 0 )
            {
                mAudioStream = formatContext->streams[mAudioStreamId];
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
            mVideoFrame = NULL;
            mAudioFrame = NULL;

            // Determine required buffer size and allocate buffer
            int videoFrameSize = 0;
            int audioFrameSize = 0;

            // Allocate space for the native video frame
            if ( mHasVideo )
            {
                mVideoFrame = av_frame_alloc();
                videoFrameSize = avpicture_get_size( mVideoCodecContext->pix_fmt, mVideoCodecContext->width, mVideoCodecContext->height );
            }

            if ( mHasAudio )
            {
                mAudioFrame = av_frame_alloc();
                audioFrameSize = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;
            }

            mVideoFrameBuffer.size( videoFrameSize );
            mAudioFrameBuffer.size( audioFrameSize );

            AVPacket packet;
            av_init_packet(&packet);
            mBaseTimestamp = time64();
            mLastTimestamp = mBaseTimestamp;
            int frameComplete = false;
            while( !mStop )
            {
                while ( !mStop && av_read_frame( formatContext, &packet ) >= 0)
                {
                    AVPacket origPacket = packet;
                    do {
                        int decoded = decodePacket( packet, frameComplete );
                        if ( decoded < 0)
                            break;
                        packet.data += decoded;
                        packet.size -= decoded;
                    } while ( packet.size > 0 );
                    av_free_packet( &origPacket );
                }
                if ( mStop )
                    break;
                packet.data = NULL;
                packet.size = 0;
                do {
                    decodePacket( packet, frameComplete );
                } while ( frameComplete );

                if ( !(formatContext->iformat->flags & AVFMT_NOFILE) )
                {
                    if ( loop )
                    {
                        Debug( 2, "Looping source..." );
                        if ( av_seek_frame( formatContext, -1, 0, AVSEEK_FLAG_ANY ) < 0 )
                        {
                            Error( "Can't loop video, seek failed" );
                            mStop = true;
                        }
                        // Artificially delay next pass by one frame interval?
                        //int64_t frameInterval = (1000000L * mVideoStream->r_frame_rate.num)/mVideoStream->r_frame_rate.den,
                        //mBaseTimestamp = mLastTimestamp+frameInterval;
                        mBaseTimestamp = mLastTimestamp;
                    }
                }
            }
            if ( mHasVideo && !mStop)
            {
                av_freep( &mVideoFrame );
                mVideoStream = NULL;
                if ( mVideoCodecContext )
                {
                    avcodec_close( mVideoCodecContext );
                    mVideoCodecContext = NULL; // Freed by avformat_close_input
                }
            }
            if ( mHasAudio && !mStop)
            {
                av_freep( &mAudioFrame );
                mAudioStream = NULL;
                if ( mAudioCodecContext )
                {
                    avcodec_close( mAudioCodecContext );
                    mAudioCodecContext = NULL; // Freed by avformat_close_input
                }
            }
            if ( formatContext && !mStop)
            {
                avformat_close_input( &formatContext );
                formatContext = NULL;
                //av_free( formatContext );
            }
            if ( loop && !mStop)
            {
                // For network input just restart from scratch as not seekable
                if ( formatContext->iformat->flags & AVFMT_NOFILE )
                {
                    Warning( "%s: Reconnecting", cidentity() );
                    sleep( RECONNECT_TIMEOUT );
                }
            }
        }
        Info ("cleanup in AVInput");
        av_dict_free(&dict);
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

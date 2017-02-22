#include "../base/oz.h"
#include "ozMovieFileOutput.h"

#include "../base/ozFeedFrame.h"
#include "../base/ozNotifyFrame.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//AVFrame *picture, *tmp_picture;
//int frame_count, video_outbuf_size;

/**
* @brief 
*
* @return 
*/
int MovieFileOutput::run()
{
    int notificationId = 0; // Hack to generate an id
    int avResult = 0;       // Store ffmpeg return values

    std::string location = mOptions.get( "location", "/tmp" );
    std::string format = mOptions.get( "format", "avi" );
    int duration = mOptions.get( "duration", 300 );

    /* auto detect the output format from the name. default is mpeg. */
    AVOutputFormat *outputFormat = av_guess_format( format.c_str(), nullptr, nullptr );
    if ( !outputFormat )
        Fatal( "Could not deduce output format from '%s'", format.c_str() );

    if ( waitForProviders() )
    {
        setReady();

        int64_t inputChannelLayout = audioProvider()->channelLayout();
        int inputChannels = av_get_channel_layout_nb_channels(inputChannelLayout);
        AVSampleFormat inputSampleFormat = audioProvider()->sampleFormat();
        int inputSampleRate = audioProvider()->sampleRate();
        uint16_t inputSamples = audioProvider()->samples();

        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat inputPixelFormat = videoProvider()->pixelFormat();
        FrameRate inputFrameRate = videoProvider()->frameRate();

        Info( "Input %d x %d (%d) @ %d/%d", inputWidth, inputHeight, inputPixelFormat, inputFrameRate.num, inputFrameRate.den );

        while( !mStop )
        {
            time_t now = time( nullptr );
            std::string filename = stringtf( "%s/%s-%ld.%s", location.c_str(), cname(), now, format.c_str() );

            /* allocate the output media context */
            AVFormatContext *outputContext = avformat_alloc_context();
            if ( !outputContext )
                Fatal( "Unable to allocate output context" );
            outputContext->oformat = outputFormat;

            AVDictionary *opts = nullptr;
            //av_dict_set( &opts, "vpre", "medium", 0 );

            av_dict_set( &outputContext->metadata, "author", "oZone MovieFileOutput", 0 );
            if ( mOptions.exists( "title" ) )
            {
                std::string option = mOptions.get( "title", "" );
                av_dict_set( &outputContext->metadata, "title", option.c_str(), 0 );
            }
            if ( mOptions.exists( "comment" ) )
            {
                std::string option = mOptions.get( "comment", "" );
                av_dict_set( &outputContext->metadata, "comment", option.c_str(), 0 );
            }
            //av_dict_set( &outputContext->metadata, "copyright", "COPYRIGHT", 0 );

            // x264 Baseline
            avSetH264Preset( &opts, "medium" );
            av_dict_set( &opts, "crf", "1", 0 );
            av_dict_set( &opts, "quality", "80", 0 );
            av_dict_set( &opts, "q", "80", 0 );
            av_dict_set( &opts, "compression_level", "10", 0 );
            //av_dict_set( &opts, "b", "200k", 0 );
            //av_dict_set( &opts, "bt", "240k", 0 );

            avDumpDict( opts );

            //
            // Video setup
            //
            AVStream *videoStream = nullptr;
            AVCodecContext *videoCodecContext = nullptr;
            SwsContext *scaleContext = nullptr;
            AVFrame *avVideoFrame = nullptr;
            AVFrame *avScaleFrame = nullptr;
            ByteBuffer scaleBuffer;

            if ( provider()->hasVideo() && (outputFormat->video_codec != CODEC_ID_NONE) )
            {
                //videoStream = av_new_stream( outputContext, 0 );
                videoStream = avformat_new_stream( outputContext, nullptr );
                if ( !videoStream )
                    Fatal( "Could not alloc video stream" );

                videoCodecContext = videoStream->codec;
                videoCodecContext->codec_id = outputFormat->video_codec;
                videoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;

                /* find the video encoder */
                AVCodec *videoCodec = avcodec_find_encoder( videoCodecContext->codec_id );
                if ( !videoCodec )
                    Fatal( "Video codec not found" );

                videoCodecContext->pix_fmt = choose_pixel_fmt( videoStream, videoCodecContext, videoCodec, mOptions.get( "pixelFormat", inputPixelFormat ) );
                videoCodecContext->width = mOptions.get( "width", (int)inputWidth );
                videoCodecContext->height = mOptions.get( "height", (int)inputHeight );
                videoCodecContext->time_base = inputFrameRate.timeBase();
                if ( mOptions.exists( "frameRate" ) )
                {
                    FrameRate frameRate = (FrameRate)mOptions.get( "frameRate", 0.0 );
                    videoCodecContext->time_base = frameRate.timeBase();
                }
                videoCodecContext->bit_rate = mOptions.get( "videoBitRate", videoCodecContext->bit_rate );
                videoStream->time_base = videoCodecContext->time_base;

                //videoCodecContext->bit_rate = videoProvider()->videoBitRate();
                //videoCodecContext->gop_size = 12; /* emit one intra frame every twelve frames at most */

                Info( "Video Output: %d x %d (%d) @ %d/%d(%d/%d)", videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt, videoCodecContext->time_base.num, videoCodecContext->time_base.den, videoStream->time_base.num, videoStream->time_base.den );

                // some formats want stream headers to be separate
                if ( outputContext->oformat->flags & AVFMT_GLOBALHEADER )
                    videoCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

                int qscale = 25;
                if ( qscale >= 0)
                {
                    videoCodecContext->flags |= CODEC_FLAG_QSCALE;
                    videoCodecContext->global_quality = FF_QP2LAMBDA * qscale;
                }

                if ( inputWidth != videoCodecContext->width || inputHeight != videoCodecContext->height || inputPixelFormat != videoCodecContext->pix_fmt )
                {
                    // Prepare for image format and size conversions
                    scaleContext = sws_getContext( inputWidth, inputHeight, inputPixelFormat, videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr );
                    if ( !scaleContext )
                        Fatal( "Unable to create scale context for encoder" );
                }

                /* open the codec */
                if ( avcodec_open2( videoCodecContext, videoCodec, &opts ) < 0 )
                    Fatal( "Could not open video codec" );
                Debug( 1, "Opened output video codec %s (%s)", videoCodec->name, videoCodec->long_name );

                avResult = avpicture_get_size( inputPixelFormat, inputWidth, inputHeight );
                if ( avResult < 0 )
                    Fatal( "Unable to get video picture size: %d", avResult );

                //videoBuffer.size( avResult );

                avVideoFrame = av_frame_alloc();
                avVideoFrame->format = inputPixelFormat;
                avVideoFrame->width = inputWidth;
                avVideoFrame->height = inputHeight;

                if ( scaleContext )
                {
                    // Make space for anything that is going to be output
                    avScaleFrame = av_frame_alloc();
                    avScaleFrame->format = videoCodecContext->pix_fmt;
                    avScaleFrame->width = videoCodecContext->width;
                    avScaleFrame->height = videoCodecContext->height;
                    avResult = avpicture_get_size( (PixelFormat)avScaleFrame->format, avScaleFrame->width, avScaleFrame->height );
                    if ( avResult < 0 )
                        Fatal( "Unable to get scaled picture size: %d", avResult );
                    scaleBuffer.size( avResult );
                    avResult = avpicture_fill( (AVPicture *)avScaleFrame, scaleBuffer.data(), (PixelFormat)avScaleFrame->format, avScaleFrame->width, avScaleFrame->height );
                    if ( avResult < 0 )
                        Fatal( "Unable to fill scale frame: %d", avResult );
                }
            }
            else
            {
                Warning( "No video" );
            }

            //
            // Audio setup
            //
            AVStream *audioStream = nullptr;
            AVCodecContext *audioCodecContext = nullptr;
            SwrContext *resampleContext = nullptr;
            AVFrame *avAudioFrame = nullptr;
            AVFrame *avResampleFrame = nullptr;
            ByteBuffer resampleBuffer;

            if ( provider()->hasAudio() && (outputFormat->audio_codec != CODEC_ID_NONE) )
            {
                //audioStream = av_new_stream( outputContext, 1 );
                audioStream = avformat_new_stream( outputContext, nullptr );
                if ( !audioStream )
                    Fatal( "Could not alloc audio stream" );

                audioCodecContext = audioStream->codec;
                audioCodecContext->codec_id = outputFormat->audio_codec;

                audioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
                audioCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

                /* find the audio encoder */
                AVCodec *audioCodec = avcodec_find_encoder( audioCodecContext->codec_id );
                if ( !audioCodec )
                    Fatal( "Audio codec not found" );

                choose_sample_fmt( audioStream, audioCodec );
                if ( mOptions.exists( "sampleFormat" ) )
                    audioCodecContext->sample_fmt = mOptions.get( "sampleFormat", audioCodecContext->sample_fmt );
                /* select other audio parameters supported by the encoder */
                audioCodecContext->sample_rate = select_sample_rate(audioCodec);
                if ( mOptions.exists( "sampleRate" ) )
                    audioCodecContext->sample_rate = mOptions.get( "sampleRate", audioCodecContext->sample_rate );
                audioCodecContext->channel_layout = select_channel_layout(audioCodec);
                if ( mOptions.exists( "channelLayout" ) )
                    audioCodecContext->channel_layout = mOptions.get( "channelLayout", audioCodecContext->channel_layout );
                audioCodecContext->channels = av_get_channel_layout_nb_channels(audioCodecContext->channel_layout);
                if ( mOptions.exists( "channels" ) )
                    audioCodecContext->channels = mOptions.get( "channels", audioCodecContext->channels );
                audioCodecContext->bit_rate = mOptions.get( "audioBitRate", audioCodecContext->bit_rate );
                audioCodecContext->time_base = { 1, audioCodecContext->sample_rate };
                audioStream->time_base = audioCodecContext->time_base;

                //Info( "Derived %d (%d) @ %d", audioCodecContext->sample_rate, audioCodecContext->channels, audioCodecContext->sample_fmt );
                if ( inputChannelLayout != audioCodecContext->channel_layout || inputSampleFormat != audioCodecContext->sample_fmt || inputSampleRate != audioCodecContext->sample_rate )
                {
                    resampleContext = swr_alloc_set_opts( NULL,              // we're allocating a new context
                                          audioCodecContext->channel_layout, // out_ch_layout
                                          audioCodecContext->sample_fmt,     // out_sample_fmt
                                          audioCodecContext->sample_rate,    // out_sample_rate
                                          inputChannelLayout,                // in_ch_layout
                                          inputSampleFormat,                 // in_sample_fmt
                                          inputSampleRate,                   // in_sample_rate
                                          0,                                 // log_offset
                                          NULL );                            // log_ctx

                    if ( !resampleContext )
                        Fatal( "Unable to create resample context for encoder" );
                }

                /* open it */
                avResult = avcodec_open2( audioCodecContext, audioCodec, &opts );
                if ( avResult < 0 )
                    Fatal( "Could not open audio codec %d: %d", audioCodecContext->codec_id, avResult );
                Debug( 1, "Opened output audio codec %s (%s)", audioCodec->name, audioCodec->long_name );

                if ( !check_sample_fmt( audioCodec, audioCodecContext->sample_fmt ) ) {
                    Fatal( "Encoder does not support sample format %s", av_get_sample_fmt_name(audioCodecContext->sample_fmt) );
                }

                avAudioFrame = av_frame_alloc();
                avAudioFrame->channel_layout = inputChannelLayout;
                avAudioFrame->channels = inputChannels;
                avAudioFrame->format = inputSampleFormat;
                avAudioFrame->sample_rate = inputSampleRate;

                if ( resampleContext )
                {
                    // Make space for anything that is going to be output
                    avResampleFrame = av_frame_alloc();
                    avResampleFrame->channel_layout = audioCodecContext->channel_layout;
                    avResampleFrame->channels = audioCodecContext->channels;
                    avResampleFrame->format = audioCodecContext->sample_fmt;
                    avResampleFrame->sample_rate = audioCodecContext->sample_rate;
                    Info( "Rate: %d -> %d", avResampleFrame->sample_rate, inputSampleRate );
                    //avResampleFrame->nb_samples = av_rescale_rnd( audioCodecContext->frame_size, avResampleFrame->sample_rate, inputSampleRate, AV_ROUND_UP );
                    avResampleFrame->nb_samples = audioCodecContext->frame_size;
                    avResult = av_samples_get_buffer_size( nullptr, avResampleFrame->channels, avResampleFrame->nb_samples, (AVSampleFormat)avResampleFrame->format, 0 );
                    if ( avResult < 0 )
                        Fatal( "Unable to get audio buffer size: %d", avResult );
                    Info( "Size: %d", avResult );
                    Info( "Samples: %d -> %d", audioCodecContext->frame_size, avResampleFrame->nb_samples );
                    Info( "Samples: %d", avAudioFrame->nb_samples );
                    resampleBuffer.size( avResult );
                    //needed_size = av_samples_get_buffer_size(NULL, nb_channels, frame->nb_samples, sample_fmt, align);

                    //Info( "avcodec_fill_audio_frame( %p, %d, %d, %p, %d, %d )", avAudioFrame, audioCodecContext->channels, audioCodecContext->sample_fmt, audioFrame->buffer().data(), audioFrame->buffer().size(), 0 );
                    avResult = avcodec_fill_audio_frame( avResampleFrame, avResampleFrame->channels, (AVSampleFormat)avResampleFrame->format, resampleBuffer.data(), resampleBuffer.size(), 0 );
                    if ( avResult < 0 )
                        Fatal( "Unable to fill resample frame: %d", avResult );
                }
            }
            else
            {
                Warning( "No audio" );
            }

#if 0
            /* set the output parameters (must be done even if no
               parameters). */
            if ( av_set_parameters( outputContext, nullptr ) < 0 )
                Fatal( "Invalid output format parameters" );
#endif

            if ( dbgLevel > DBG_INF )
                av_dump_format( outputContext, 0, filename.c_str(), 1 );

            snprintf( outputContext->filename, sizeof(outputContext->filename), "%s", filename.c_str() );
            Info( "Writing to movie file '%s'", outputContext->filename );

            /* open the output file, if needed */
            if ( !(outputFormat->flags & AVFMT_NOFILE) )
            {
                if ( avio_open( &outputContext->pb, filename.c_str(), AVIO_FLAG_WRITE ) < 0 )
                {
                    Fatal( "Could not open output filename '%s'", filename.c_str() );
                }
                DiskIONotification::DiskIODetail detail( DiskIONotification::DiskIODetail::WRITE, DiskIONotification::DiskIODetail::BEGIN, filename );
                DiskIONotification *notification = new DiskIONotification( this, ++notificationId, detail );
                distributeFrame( FramePtr( notification ) );
            }

            /* write the stream header, if any */
            avformat_write_header( outputContext, &opts );
            avDumpDict( opts );

            bool fileComplete = false;
            int gotPacket = 0;
            double audioTimeOffset = 0.0L, videoTimeOffset = 0.0L;
            uint64_t audioFrameCount = 0, audioSampleCount = 0, videoFrameCount = 0;
            ByteBuffer audioBuffer;
            AVPacket pkt;
            while( !mStop )
            {
                mQueueMutex.lock();
                if ( !mFrameQueue.empty() )
                {
                    for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                    {
                        if ( ( !audioStream || (audioTimeOffset >= duration) ) && ( !videoStream || (videoTimeOffset >= duration) ) )
                            fileComplete = true;

                        const FeedFrame *frame = iter->get();
                        /* write interleaved audio and video frames */
                        //if ( !videoStream || (videoStream && audioStream && audioTimeOffset < videoTimeOffset) )
                        if ( audioStream && frame->mediaType() == FeedFrame::FRAME_TYPE_AUDIO )
                        {
                            const AudioFrame *audioFrame = dynamic_cast<const AudioFrame *>(frame);

                            avAudioFrame->nb_samples = audioFrame->samples();
                            avResult = av_samples_get_buffer_size( nullptr, audioFrame->channels(), audioFrame->samples(), audioFrame->sampleFormat(), 0 );
                            Info( "Size: %d, Buffer size: %d", avResult, audioFrame->buffer().size() );
                            avResult = avcodec_fill_audio_frame( avAudioFrame, audioFrame->channels(), audioFrame->sampleFormat(), audioFrame->buffer().data(), audioFrame->buffer().size(), 0 );
                            if ( avResult < 0 )
                                Fatal( "Unable to fill audio frame: %s (%d)", avStrError(avResult), avResult );

                            AVFrame *avOutputFrame = avAudioFrame;
                            if ( resampleContext )
                            {
                                int64_t outputDelay = swr_get_delay( resampleContext, inputSampleRate );
                                int outputSamples = av_rescale_rnd( outputDelay + audioFrame->samples(), audioCodecContext->sample_rate, inputSampleRate, AV_ROUND_UP );

                                Info( "OD: %jd, OS: %d", outputDelay, outputSamples );
                                //avResampleFrame->nb_samples = avAudioFrame->nb_samples;
                                avResampleFrame->nb_samples = outputSamples;
                                avResult = av_samples_get_buffer_size( nullptr, avResampleFrame->channels, avResampleFrame->nb_samples, (AVSampleFormat)avResampleFrame->format, 0 );
                                if ( avResult < 0 )
                                    Fatal( "Unable to get audio buffer size: %d", avResult );
                                resampleBuffer.size( avResult );
                                avResult = avcodec_fill_audio_frame( avResampleFrame, avResampleFrame->channels, (AVSampleFormat)avResampleFrame->format, resampleBuffer.data(), resampleBuffer.size(), 0 );
                                if ( avResult < 0 )
                                    Fatal( "Unable to fill resample frame: %d", avResult );
                                avResult = swr_convert_frame( resampleContext, avResampleFrame, avAudioFrame );
                                if ( avResult < 0 )
                                    Fatal( "Unable to resample input frame (%d@%dx%d) to output frame (%d@%dx%d) at frame %ju", inputPixelFormat, inputWidth, inputHeight, videoCodecContext->pix_fmt, videoCodecContext->width, videoCodecContext->height, videoFrameCount );
                                avOutputFrame = avResampleFrame;
                            }
                            //avOutputFrame->pts = audioSampleCount;

                            size_t outputFrameSize = av_samples_get_buffer_size( nullptr, avOutputFrame->channels, avOutputFrame->nb_samples, (AVSampleFormat)avOutputFrame->format, 0 );
                            size_t requiredFrameSize = av_samples_get_buffer_size( nullptr, avOutputFrame->channels, audioCodecContext->frame_size, (AVSampleFormat)avOutputFrame->format, 0 );
                            Info( "OFS: %d", outputFrameSize );
                            audioBuffer.append( avOutputFrame->extended_data[0], outputFrameSize );
                            /*Info( "OF: %p/%p/%p, %d/%d/%d",
                                avOutputFrame->extended_data,
                                avOutputFrame->extended_data[0],
                                avOutputFrame->extended_data[1],
                                avOutputFrame->linesize[0],
                                avOutputFrame->linesize[1],
                                avOutputFrame->linesize[2]
                            );*/

                            Info( "ABS: %d, RFS: %d", audioBuffer.size(), requiredFrameSize );
                            while ( audioBuffer.size() >= requiredFrameSize )
                            {
                                Debug( 1, "Encoding audio packet at frame %d, pts %jd", audioCodecContext->frame_number, avOutputFrame->pts );

                                avOutputFrame->nb_samples = audioCodecContext->frame_size;
                                avResult = avcodec_fill_audio_frame( avOutputFrame, avOutputFrame->channels, (AVSampleFormat)avOutputFrame->format, audioBuffer.data(), requiredFrameSize, 0 );
                                if ( avResult < 0 )
                                    Fatal( "Unable to fill resample frame: %d", avResult );
                                //size_t frameSamples = requiredFrameSize/avOutputFrame->channels;
                                size_t frameSamples = avOutputFrame->nb_samples;
                                Info( "FS: %d", frameSamples );
                                AVFrame *encodeFrame = avOutputFrame;
                                encodeFrame->pts = audioSampleCount;
                                do
                                {
                                    av_init_packet( &pkt );
                                    avResult = avcodec_encode_audio2( audioCodecContext, &pkt, encodeFrame, &gotPacket );
                                    if ( avResult < 0 )
                                    {
                                        Error( "Error encoding audio frame %d", avResult );
                                        continue;
                                    }

                                    /* write the compressed frame in the media file */
                                    if ( gotPacket )
                                    {
                                        pkt.stream_index = audioStream->index;

                                        av_pkt_dump2( nullptr, &pkt, 0, audioStream );

                                        av_packet_rescale_ts( &pkt, audioCodecContext->time_base, audioStream->time_base );

                                        //pkt.pts = pkt.dts = audioSampleCount;
                                        audioTimeOffset = pkt.pts * av_q2d(audioStream->time_base);

                                        //if ( pkt.pts != AV_NOPTS_VALUE )
                                            //pkt.pts = pkt.dts = av_rescale_q( pkt.pts, audioCodecContext->time_base, audioStream->time_base );
                                        // ???
                                        //if ( pkt.pts != AV_NOPTS_VALUE )
                                            //audioTimeOffset = (pkt.pts * (double)videoCodecContext->time_base.num) / videoCodecContext->time_base.den;
                                        av_pkt_dump2( nullptr, &pkt, 0, audioStream );
                                        Debug( 1, "ASC: %d, ATO: %.2lf, PTS: %jd, DTS: %jd", audioSampleCount, audioTimeOffset, pkt.pts, pkt.dts );
                                        //???
                                        //pkt.flags |= AV_PKT_FLAG_KEY;
                                        //pkt.data = audioBuffer.data();

                                        avResult = av_interleaved_write_frame( outputContext, &pkt );
                                        if ( avResult < 0 )
                                            Fatal( "Error writing audio frame: %s (%d)", avStrError(avResult), avResult );
                                        encodeFrame = nullptr;
                                        av_free_packet( &pkt );
                                    }
                                } while ( fileComplete && gotPacket == 1 && avResult == 0 );
                                audioSampleCount += frameSamples;
                                audioBuffer.consume( requiredFrameSize );
                                Info( "ABS: %d", audioBuffer.size() );
                            }
                            audioFrameCount++;
                        }
                        else if ( videoStream && frame->mediaType() == FeedFrame::FRAME_TYPE_VIDEO )
                        {
                            const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame);

                            avResult = avpicture_fill( (AVPicture *)avVideoFrame, videoFrame->buffer().data(), inputPixelFormat, inputWidth, inputHeight );
                            if ( avResult < 0 )
                                Fatal( "Unable to fill video frame: %d", avResult );

                            AVFrame *avOutputFrame = avVideoFrame;
                            if ( scaleContext )
                            {
                                if ( sws_scale( scaleContext, avVideoFrame->data, avVideoFrame->linesize, 0, inputHeight, avScaleFrame->data, avScaleFrame->linesize ) < 0 )
                                    Fatal( "Unable to convert input frame (%d@%dx%d) to output frame (%d@%dx%d) at frame %ju", inputPixelFormat, inputWidth, inputHeight, videoCodecContext->pix_fmt, videoCodecContext->width, videoCodecContext->height, videoFrameCount );
                                avOutputFrame = avScaleFrame;
                                avScaleFrame->pts = avVideoFrame->pts;
                            }
                            avOutputFrame->pts = videoFrameCount;
                            //avOutputFrame->pts = av_rescale_q( videoFrameCount, videoCodecContext->time_base, inputFrameRate.timeBase() );
                            //avOutputFrame->pts = av_rescale_q( videoFrameCount, inputFrameRate.timeBase(), videoCodecContext->time_base );

                            do
                            {
                                av_init_packet( &pkt );
                                avResult = avcodec_encode_video2( videoCodecContext, &pkt, avOutputFrame, &gotPacket );
                                if ( avResult < 0 )
                                {
                                    Error( "Error encoding video frame %d", avResult );
                                    break;
                                }

                                if ( gotPacket )
                                {
                                    pkt.stream_index = videoStream->index;

                                    Info( "PTS: %jd", pkt.pts );
                                    if ( pkt.pts != AV_NOPTS_VALUE )
                                        videoTimeOffset = (pkt.pts * (double)videoCodecContext->time_base.num) / videoCodecContext->time_base.den;
                                    Debug( 1, "VTO: %.2lf", videoTimeOffset );
                                    //videoTimeOffset = videoCodecContext->coded_frame->pts/1000000.0L;
                                    //videoTimeOffset = (double)videoStream->pts.val + ((double)videoStream->pts.num / (double)videoStream->pts.den);
                                    //videoTimeOffset = (double)videoCodecContext->coded_frame->pts * videoStream->time_base.den / videoStream->time_base.num;
                                    av_packet_rescale_ts( &pkt, videoCodecContext->time_base, videoStream->time_base );
                                    //if ( pkt.pts != AV_NOPTS_VALUE )
                                        //pkt.pts = av_rescale_q( pkt.pts, videoCodecContext->time_base, videoStream->time_base );
                                    //pkt.dts = pkt.pts;
                                    av_pkt_dump2( nullptr, &pkt, 0, videoStream );
                                    Info( "PTS2: %jd", pkt.pts );
                                    //pkt.stream_index = videoStream->index;

                                    /* write the compressed frame in the media file */
                                    avResult = av_interleaved_write_frame( outputContext, &pkt );
                                    if ( avResult != 0 )
                                        Fatal( "Error while writing video frame: %d", avResult );
                                    avOutputFrame = nullptr;
                                    av_free_packet( &pkt );
                                }
                            } while ( fileComplete && gotPacket == 1 && avResult == 0 );
                            videoFrameCount++;
                        }
                    }
                    mFrameQueue.clear();
                }
                mQueueMutex.unlock();
                checkProviders();
                if ( fileComplete )
                    break;
                usleep( INTERFRAME_TIMEOUT );
            }

            /* write the trailer, if any.  the trailer must be written
             * before you close the CodecContexts open when you wrote the
             * header; otherwise write_trailer may try to use memory that
             * was freed on av_codec_close() */
            av_write_trailer( outputContext );

            /* close each codec */
            if ( videoStream )
            {
                if ( scaleContext )
                {
                    sws_freeContext( scaleContext );
                    av_free( avScaleFrame );
                }
                avcodec_close( videoStream->codec );
                av_free( avVideoFrame );
            }
            if ( audioStream )
            {
                if ( resampleContext )
                {
                    swr_free( &resampleContext );
                    av_free( avResampleFrame );
                }
                avcodec_close( audioStream->codec );
                av_free( avAudioFrame );
            }

            /* free the streams */
            for ( unsigned int i = 0; i < outputContext->nb_streams; i++ )
            {
                av_freep( &outputContext->streams[i]->codec );
                av_freep( &outputContext->streams[i] );
            }

            if ( !(outputFormat->flags & AVFMT_NOFILE) )
            {
                size_t fileSize = avio_size( outputContext->pb );

                /* close the output file */
                avio_close( outputContext->pb );

                DiskIONotification::DiskIODetail detail( filename, fileSize );
                DiskIONotification *notification = new DiskIONotification( this, ++notificationId, detail );
                distributeFrame( FramePtr( notification ) );
            }

            /* free the stream */
            av_free( outputContext );
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return 0;
}

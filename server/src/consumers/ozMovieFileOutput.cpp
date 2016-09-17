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

    if ( waitForProviders() )
    {
        setReady();

        // Find the source codec context
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat inputPixelFormat = videoProvider()->pixelFormat();
        FrameRate inputFrameRate = videoProvider()->frameRate();

        Debug(1, "Provider framerate = %d/%d", inputFrameRate.num, inputFrameRate.den );

        /* auto detect the output format from the name. default is mpeg. */
        AVOutputFormat *outputFormat = av_guess_format( mFormat.c_str(), NULL, NULL );
        if ( !outputFormat )
            Fatal( "Could not deduce output format from '%s'", mFormat.c_str() );

        while( !mStop )
        {
            /* allocate the output media context */
            AVFormatContext *outputContext = avformat_alloc_context();
            if ( !outputContext )
                Fatal( "Unable to allocate output context" );
            outputContext->oformat = outputFormat;

            AVDictionary *opts = NULL;
            //av_dict_set( &opts, "width", "352", 0 );
            //av_dict_set( &opts, "height", "288", 0 );
            //av_dict_set( &opts, "vpre", "medium", 0 );

            av_dict_set( &outputContext->metadata, "author", "AUTHOR", 0 );
            av_dict_set( &outputContext->metadata, "comment", "COMMENT", 0 );
            av_dict_set( &outputContext->metadata, "copyright", "COPYRIGHT", 0 );
            av_dict_set( &outputContext->metadata, "title", "TITLE", 0 );

            // x264 Baseline
            avSetH264Preset( &opts, "medium" );
            av_dict_set( &opts, "b", "200k", 0 );
            av_dict_set( &opts, "bt", "240k", 0 );

            avDumpDict( opts );

            /* add the audio and video streams using the default format codecs and initialize the codecs */
            AVStream *videoStream = NULL;
            AVCodecContext *videoCodecContext = NULL;
            if ( provider()->hasVideo() && (outputFormat->video_codec != CODEC_ID_NONE) )
            {
                //videoStream = av_new_stream( outputContext, 0 );
                videoStream = avformat_new_stream( outputContext, NULL );
                if ( !videoStream )
                    Fatal( "Could not alloc video stream" );

                videoCodecContext = videoStream->codec;
                videoCodecContext->codec_id = outputFormat->video_codec;
                videoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;

                videoCodecContext->width = mVideoParms.width();
                videoCodecContext->height = mVideoParms.height();
                videoCodecContext->time_base = mVideoParms.frameRate();
                videoCodecContext->time_base.num = 1;
                videoCodecContext->time_base.den = 25;
                videoCodecContext->pix_fmt = mVideoParms.pixelFormat();
#if 0
                /* put sample parameters */
                videoCodecContext->bit_rate = mVideoParms.bitRate();
                /* resolution must be a multiple of two */
                videoCodecContext->width = mVideoParms.width();
                videoCodecContext->height = mVideoParms.height();
                /* time base: this is the fundamental unit of time (in seconds) in terms
                   of which frame timestamps are represented. for fixed-fps content,
                   timebase should be 1/framerate and timestamp increments should be
                   identically 1. */
                videoCodecContext->time_base = mVideoParms.frameRate();
                videoCodecContext->gop_size = 12; /* emit one intra frame every twelve frames at most */
                videoCodecContext->pix_fmt = mVideoParms.pixelFormat();
#endif

                // some formats want stream headers to be separate
                if ( outputContext->oformat->flags & AVFMT_GLOBALHEADER )
                    videoCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }

            struct SwsContext *convertContext = NULL;
            if ( inputWidth != mVideoParms.width() || inputHeight != mVideoParms.height() || inputPixelFormat != mVideoParms.pixelFormat() )
            {
                // Prepare for image format and size conversions
                convertContext = sws_getContext( inputWidth, inputHeight, inputPixelFormat, mVideoParms.width(), mVideoParms.height(), mVideoParms.pixelFormat(), SWS_BICUBIC, NULL, NULL, NULL );
                if ( !convertContext )
                    Fatal( "Unable to create conversion context for encoder" );
            }

            AVStream *audioStream = NULL;
            AVCodecContext *audioCodecContext = NULL;
            if ( provider()->hasAudio() && (outputFormat->audio_codec != CODEC_ID_NONE) )
            {
                //audioStream = av_new_stream( outputContext, 1 );
                audioStream = avformat_new_stream( outputContext, NULL );
                if ( !audioStream )
                    Fatal( "Could not alloc audio stream" );

                audioCodecContext = audioStream->codec;
                audioCodecContext->codec_id = outputFormat->audio_codec;
                    setReady();

                audioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;

                /* put sample parameters */
                //audioCodecContext->bit_rate = mAudioParms.bitRate();
                //audioCodecContext->sample_rate = mAudioParms.sampleRate();
                //audioCodecContext->channels = mAudioParms.channels();
            }

#if 0
            /* set the output parameters (must be done even if no
               parameters). */
            if ( av_set_parameters( outputContext, NULL ) < 0 )
                Fatal( "Invalid output format parameters" );
#endif

            //av_dump_format( outputContext, 0, filename, 1 );

            /* now that all the parameters are set, we can open the audio and
               video codecs and allocate the necessary encode buffers */
            AVFrame *avInputFrame = NULL;
            AVFrame *avInterFrame = NULL;
            ByteBuffer interBuffer;
            ByteBuffer videoBuffer;
            ByteBuffer audioBuffer;
            if ( videoStream )
            {
                /* find the video encoder */
                AVCodec *codec = avcodec_find_encoder( videoCodecContext->codec_id );
                if ( !codec )
                    Fatal( "Video codec not found" );

                /* open the codec */
                if ( avcodec_open2( videoCodecContext, codec, &opts ) < 0 )
                    Fatal( "Could not open video codec" );

                avInputFrame = avcodec_alloc_frame();

                if ( !(outputContext->oformat->flags & AVFMT_RAWPICTURE) )
                {
                    videoBuffer.size( 256000 ); // Guess?
                }
                if ( convertContext )
                {
                    // Make space for anything that is going to be output
                    avInterFrame = avcodec_alloc_frame();
                    interBuffer.size( avpicture_get_size( mVideoParms.pixelFormat(), mVideoParms.width(), mVideoParms.height() ) );
                    avpicture_fill( (AVPicture *)avInterFrame, interBuffer.data(), mVideoParms.pixelFormat(), mVideoParms.width(), mVideoParms.height() );
                }
            }
            if ( audioStream )
            {
                /* find the audio encoder */
                AVCodec *codec = avcodec_find_encoder( audioCodecContext->codec_id );
                if ( !codec )
                    Fatal( "Audio codec not found" );

                /* open it */
                if ( avcodec_open2( audioCodecContext, codec, &opts ) < 0 )
                    Fatal( "Could not open audio codec" );

                audioBuffer.size( 8192 );
            }

            time_t now = time( NULL );
            std::string filename = stringtf( "%s/%s-%ld.%s", mLocation.c_str(), cname(), now, mFormat.c_str() );
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
            double audioTimeOffset = 0.0L, videoTimeOffset = 0.0L;
            uint64_t videoFrameCount = 0, audioFrameCount = 0;
            while( !mStop )
            {
                mQueueMutex.lock();
                if ( !mFrameQueue.empty() )
                {
                    for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                    {
                        /* compute current audio and video time */
                        if ( audioStream )
                            audioTimeOffset = (double)audioStream->pts.val * audioStream->time_base.num / audioStream->time_base.den;

                        if ( videoStream )
                            videoTimeOffset += (double)inputFrameRate.den / inputFrameRate.num;
                            //videoTimeOffset = videoCodecContext->coded_frame->pts/1000000.0L;
                            //videoTimeOffset = (double)videoStream->pts.val + ((double)videoStream->pts.num / (double)videoStream->pts.den);
                            //videoTimeOffset = (double)videoCodecContext->coded_frame->pts * videoStream->time_base.den / videoStream->time_base.num;

                        if ( ( !audioStream || (audioTimeOffset >= mMaxLength) ) && ( !videoStream || (videoTimeOffset >= mMaxLength) ) )
                            fileComplete = true;
                        Debug(1, "PTS: %lf, %jd, %jd", videoTimeOffset, videoCodecContext->coded_frame->pts, videoStream->pts.val );

                        const FeedFrame *frame = iter->get();
                        /* write interleaved audio and video frames */
                        //if ( !videoStream || (videoStream && audioStream && audioTimeOffset < videoTimeOffset) )
                        if ( frame->mediaType() == FeedFrame::FRAME_TYPE_AUDIO )
                        {
                            const AudioFrame *audioFrame = dynamic_cast<const AudioFrame *>(frame);

                            AVPacket pkt;
                            av_init_packet( &pkt );

                            pkt.size = avcodec_encode_audio( audioCodecContext, audioBuffer.data(), audioBuffer.size(), (const int16_t *)audioFrame->buffer().data() );

                            if ( audioCodecContext->coded_frame->pts != AV_NOPTS_VALUE )
                                pkt.pts = av_rescale_q( audioCodecContext->coded_frame->pts, audioCodecContext->time_base, audioStream->time_base );
                            pkt.flags |= AV_PKT_FLAG_KEY;
                            pkt.stream_index = audioStream->index;
                            pkt.data = audioBuffer.data();

                            /* write the compressed frame in the media file */
                            if ( av_interleaved_write_frame( outputContext, &pkt ) != 0)
                                Fatal( "Error writing audio frame" );
                            audioFrameCount++;
                        }
                        else
                        {
                            const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame);
                            Debug(1, "PF:%d @ %dx%d", videoFrame->pixelFormat(), videoFrame->width(), videoFrame->height() );

                            avpicture_fill( (AVPicture *)avInputFrame, videoFrame->buffer().data(), inputPixelFormat, inputWidth, inputHeight );
                            // XXX - Hack???
                            avInputFrame->pts = (videoFrameCount * inputFrameRate.den * videoCodecContext->time_base.den) / (inputFrameRate.num * videoCodecContext->time_base.num);

                            AVFrame *avOutputFrame = avInputFrame;
#if 0
                            if ( videoFrameCount >= STREAM_NB_FRAMES )
                            {
                                /* no more frame to compress. The codec has a latency of a few
                                   frames if using B frames, so we get the last frames by
                                   passing the same picture again */
                            }
                            else
#endif
                            {
                                if ( convertContext )
                                {
                                    if ( sws_scale( convertContext, avInputFrame->data, avInputFrame->linesize, 0, inputHeight, avInterFrame->data, avInterFrame->linesize ) < 0 )
                                        Fatal( "Unable to convert input frame (%d@%dx%d) to output frame (%d@%dx%d) at frame %ju", inputPixelFormat, inputWidth, inputHeight, videoCodecContext->pix_fmt, videoCodecContext->width, videoCodecContext->height, videoFrameCount );
                                    avOutputFrame = avInterFrame;
                                    avInterFrame->pts = avInputFrame->pts;
                                }
                            }
                            int result = 0;
                            if ( outputContext->oformat->flags & AVFMT_RAWPICTURE )
                            {
                                Debug(1, "Raw frame" );
                                /* raw video case. The API will change slightly in the near futur for that */
                                AVPacket pkt;
                                av_init_packet(&pkt);

                                pkt.flags |= AV_PKT_FLAG_KEY;
                                pkt.stream_index = videoStream->index;
                                pkt.data = (uint8_t *)avOutputFrame;
                                pkt.size = sizeof(AVPicture);

                                result = av_interleaved_write_frame( outputContext, &pkt );
                            }
                            else
                            {
                                Debug( 1,"Encoded frame" );
                                int outSize = 0;
                                do
                                {
                                    /* encode the image */
                                    outSize = avcodec_encode_video( videoCodecContext, videoBuffer.data(), videoBuffer.capacity(), avOutputFrame );
                                    /* if zero size, it means the image was buffered */
                                    Debug(1, "Outsize: %d", outSize );
                                    if ( outSize > 0 )
                                    {
                                        AVPacket pkt;
                                        av_init_packet(&pkt);

                                        if ( videoCodecContext->coded_frame->pts != AV_NOPTS_VALUE )
                                            pkt.pts = av_rescale_q( videoCodecContext->coded_frame->pts, videoCodecContext->time_base, videoStream->time_base );
                                        if ( videoCodecContext->coded_frame->key_frame )
                                            pkt.flags |= AV_PKT_FLAG_KEY;
                                        pkt.stream_index = videoStream->index;
                                        pkt.data = videoBuffer.data();
                                        pkt.size = outSize;

                                        /* write the compressed frame in the media file */
                                        result = av_interleaved_write_frame( outputContext, &pkt );
                                    }
                                    avOutputFrame = NULL;
                                } while ( fileComplete && outSize > 0 && result == 0 );
                            }
                            if ( result != 0 )
                                Fatal( "Error while writing video frame: %d", result );
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
                avcodec_close( videoStream->codec );
                av_free( avInputFrame );
                av_free( avInterFrame );
            }
            if ( audioStream )
            {
                avcodec_close( audioStream->codec );
            }

            /* free the streams */
            for ( int i = 0; i < outputContext->nb_streams; i++ )
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

#include "../base/oz.h"
#include "ozMp4FileOutput.h"

#include "../base/ozFeedFrame.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozMotionFrame.h"
#include "../libgen/libgenTime.h"

/**
* @brief 
*
* @param outputFormat
*
* @return 
*/
AVFormatContext *Mp4FileOutput::openFile( AVOutputFormat *outputFormat )
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

    av_dict_set( &outputContext->metadata, "author", "Pontis EMC Systems", 0 );
    av_dict_set( &outputContext->metadata, "comment", "PECOS Footage", 0 );
    //av_dict_set( &outputContext->metadata, "copyright", "COPYRIGHT", 0 );
    av_dict_set( &outputContext->metadata, "title", "TITLE", 0 );

    // x264 Baseline
    //avSetH264Preset( &opts, "medium" );
    //av_dict_set( &opts, "b", "200k", 0 );
    //av_dict_set( &opts, "bt", "240k", 0 );

    avDumpDict( opts );

    /* add the audio and video streams using the default format codecs and initialize the codecs */
    AVStream *videoStream = NULL;
    AVCodecContext *videoCodecContext = NULL;

    videoStream = avformat_new_stream( outputContext, NULL );
    if ( !videoStream )
        Fatal( "Could not alloc video stream" );

    videoCodecContext = videoStream->codec;
    videoCodecContext->codec_id = outputFormat->video_codec;
    videoCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;

    videoCodecContext->width = mVideoParms.width();
    videoCodecContext->height = mVideoParms.height();
    videoCodecContext->time_base = mVideoParms.frameRate();
    //videoCodecContext->time_base.num = 1;
    //videoCodecContext->time_base.den = 90000;
    //videoCodecContext->pix_fmt = mVideoParms.pixelFormat();

    // some formats want stream headers to be separate
    if ( outputContext->oformat->flags & AVFMT_GLOBALHEADER )
        videoCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

    //av_dump_format( outputContext, 0, filename, 1 );

    time_t now = time( NULL );
    std::string filename = stringtf( "%s/%s-%ld.%s", mLocation.c_str(), cname(), now, mExtension.c_str() );
    snprintf( outputContext->filename, sizeof(outputContext->filename), "%s", filename.c_str() );
    Info( "Writing to movie file '%s'", outputContext->filename );

    if ( avio_open( &outputContext->pb, filename.c_str(), AVIO_FLAG_WRITE ) < 0 )
        Fatal( "Could not open output filename '%s'", filename.c_str() );

    /* write the stream header, if any */
    avformat_write_header( outputContext, &opts );
    avDumpDict( opts );

    return( outputContext );
}

/**
* @brief 
*
* @param outputContext
*/
void Mp4FileOutput::closeFile( AVFormatContext *outputContext )
{
    /* write the trailer, if any.  the trailer must be written
     * before you close the CodecContexts open when you wrote the
     * header; otherwise write_trailer may try to use memory that
     * was freed on av_codec_close() */
    av_write_trailer( outputContext );

    for ( int i = 0; i < outputContext->nb_streams; i++ )
    {
        /* close each codec */
        avcodec_close( outputContext->streams[i]->codec );
        /* free the streams */
        av_freep( &outputContext->streams[i]->codec );
        av_freep( &outputContext->streams[i] );
    }

    /* close the output file */
    avio_close( outputContext->pb );

    /* free the stream */
    av_free( outputContext );
}

/**
* @brief 
*
* @return 
*/
int Mp4FileOutput::run()
{
    //const int MAX_EVENT_HEAD_AGE = 2;    ///< Number of seconds of video before event to save
    const int MAX_EVENT_TAIL_AGE = 3;    ///< Number of seconds of video after event to save

    typedef enum { IDLE, PREALARM, ALARM, ALERT } AlarmState;

    if ( waitForProviders() )
    {
        /* auto detect the output format from the name. default is mpeg. */
        AVOutputFormat *outputFormat = av_guess_format( mExtension.c_str(), NULL, NULL );
        if ( !outputFormat )
            Fatal( "Could not deduce output format from '%s'", mExtension.c_str() );
        //AVFormatContext *outputContext = openFile( outputFormat );
        AVFormatContext *outputContext = NULL;

        double videoTimeOffset = 0.0L;
        uint64_t videoFrameCount = 0;
        AlarmState alarmState = IDLE;
        uint64_t alarmTime = 0;
        int eventCount = 0;
        while( !mStop )
        {
            while( !mStop )
            {
                mQueueMutex.lock();
                if ( !mFrameQueue.empty() )
                {
                    for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                    {
                        const FeedFrame *frame = iter->get();

                        Debug( 3, "Frame type %d", frame->mediaType() );
                        if ( frame->mediaType() == FeedFrame::FRAME_TYPE_VIDEO )
                        {
                        	// This is an alarm detection frame
                            const MotionFrame *motionFrame = dynamic_cast<const MotionFrame *>(frame);
                            //const VideoProvider *provider = dynamic_cast<const VideoProvider *>(frame->provider());

                            AlarmState lastAlarmState = alarmState;
                            uint64_t now = time64();

                            Debug( 3, "Motion frame, alarmed %d", motionFrame->alarmed() );
                            if ( motionFrame->alarmed() )
                            {
                                alarmState = ALARM;
                                alarmTime = now;
                                if ( lastAlarmState == IDLE )
                                {
                                    // Create new event
                                    eventCount++;
                                    std::string path = stringtf( "%s/img-%s-%d-%ju.jpg", mLocation.c_str(), mName.c_str(), eventCount, motionFrame->id() );
                                    //Info( "PF:%d @ %dx%d", motionFrame->pixelFormat(), motionFrame->width(), motionFrame->height() );
                                    Image image( motionFrame->pixelFormat(), motionFrame->width(), motionFrame->height(), motionFrame->buffer().data() );
                                    image.writeJpeg( path.c_str() );
                                }
                            }
                            else if ( lastAlarmState == ALARM )
                            {
                                alarmState = ALERT;
                            }
                            else if ( lastAlarmState == ALERT )
                            {
                            	Debug( 3, "Frame age %.2lf", frame->age( alarmTime ) );
                                if ( (0.0l-frame->age( alarmTime )) > MAX_EVENT_TAIL_AGE )
                                    alarmState = IDLE;
                            }
                            else
                            {
                            	alarmState = IDLE;
                            }
                            Debug( 3, "Alarm state %d (%d)", alarmState, lastAlarmState );
                        }
                        else
                        {
							bool keyFrame = false;
							const uint8_t *startPos = h264StartCode( frame->buffer().head(), frame->buffer().tail() );
							while ( startPos < frame->buffer().tail() )
							{
								while( !*(startPos++) )
									;
								const uint8_t *nextStartPos = h264StartCode( startPos, frame->buffer().tail() );

								int frameSize = nextStartPos-startPos;

								unsigned char type = startPos[0] & 0x1F;
								unsigned char nri = startPos[0] & 0x60;
								Debug( 3, "Frame Type %d, NRI %d (%02x), %d bytes, ts %jd", type, nri>>5, startPos[0], frameSize, frame->timestamp() );

								if ( type == NAL_IDR_SLICE )
									keyFrame = true;
								startPos = nextStartPos;
							}

							videoTimeOffset += (double)mVideoParms.frameRate().num / mVideoParms.frameRate().den;

							if ( keyFrame )
							{
								// We can do file opening/closing now
								if ( alarmState != IDLE && !outputContext )
								{
									outputContext = openFile( outputFormat );
									videoTimeOffset = 0.0L;
									videoFrameCount = 0;
								}
								else if ( alarmState == IDLE && outputContext )
								{
									closeFile( outputContext );
									outputContext = NULL;
								}
							}
							/*if ( keyFrame && (videoTimeOffset >= mMaxLength) )
							{
								closeFile( outputContext );
								outputContext = openFile( outputFormat );
								videoTimeOffset = 0.0L;
								videoFrameCount = 0;
							}*/

							if ( outputContext )
							{
								AVStream *videoStream = outputContext->streams[0];
								AVCodecContext *videoCodecContext = videoStream->codec;

								AVPacket packet;
								av_init_packet(&packet);

								packet.flags |= keyFrame ? AV_PKT_FLAG_KEY : 0;
								packet.stream_index = videoStream->index;
								packet.data = (uint8_t*)frame->buffer().data();
								packet.size = frame->buffer().size();
								//packet.pts = packet.dts = AV_NOPTS_VALUE;
								packet.pts = packet.dts = (videoFrameCount * mVideoParms.frameRate().num * videoCodecContext->time_base.den) / (mVideoParms.frameRate().den * videoCodecContext->time_base.num);
								Debug(1, "vfc: %ju, vto: %.2lf, kf: %d, pts: %jd", videoFrameCount, videoTimeOffset, keyFrame, packet.pts );

								int result = av_interleaved_write_frame(outputContext, &packet);
								if ( result != 0 )
									Fatal( "Error while writing video frame: %d", result );
							}

							videoFrameCount++;
                        }
                    }
                    mFrameQueue.clear();
                }
                mQueueMutex.unlock();
                checkProviders();
                usleep( INTERFRAME_TIMEOUT );
            }
        }
        if ( outputContext )
        	closeFile( outputContext );
    }
    cleanup();
    return 0;
}

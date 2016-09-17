#include "../base/oz.h"
#include "ozAVFilter.h"

#include "../base/ozFeedFrame.h"
#include "../base/ozFfmpeg.h"

/**
* @brief 
*
* @param name
* @param pixelFormat
* @param width
* @param height
*/
VideoFilter::VideoFilter( const std::string &name, const std::string &filter ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mFilter( filter ),
    mBufferSrcContext( NULL ),
    mBufferSinkContext( NULL ),
    mFilterGraph( NULL )
{
}

/**
* @brief 
*
* @param pixelFormat
* @param width
* @param height
* @param provider
* @param link
*/
VideoFilter::VideoFilter( const std::string &filter, VideoProvider &provider, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mFilter( filter ),
    mBufferSrcContext( NULL ),
    mBufferSinkContext( NULL ),
    mFilterGraph( NULL )
{
}

/**
* @brief 
*/
VideoFilter::~VideoFilter()
{
    //if ( mScaleContext )
        //sws_freeContext( mScaleContext );
    mBufferSrcContext = NULL;
    mBufferSinkContext = NULL;
    mFilterGraph = NULL;
}

static const AVFilterLink *lastFilterLink( AVFilterContext *bufferSinkContext )
{
    if ( !bufferSinkContext )
        throw Exception( "No buffer sink context" );
    if ( !bufferSinkContext->nb_inputs )
        throw Exception( "No buffer sink inputs" );
    int numInputs = bufferSinkContext->nb_inputs;
    const AVFilterLink *filterLink = bufferSinkContext->inputs[numInputs-1];
    if ( !filterLink )
        throw Exception( "No buffer sink input" );
    return( filterLink );
}

uint16_t VideoFilter::width() const
{
    return( lastFilterLink( mBufferSinkContext )->w );
}

uint16_t VideoFilter::height() const
{
    return( lastFilterLink( mBufferSinkContext )->h );
}

AVPixelFormat VideoFilter::pixelFormat() const
{
    return( (AVPixelFormat)lastFilterLink( mBufferSinkContext )->format );
}

FrameRate VideoFilter::frameRate() const
{
    return( lastFilterLink( mBufferSinkContext )->frame_rate );
}

/**
* @brief 
*
* @return 
*/
int VideoFilter::run()
{
    AVFrame *inputFrame = av_frame_alloc();
    AVFrame *filterFrame = av_frame_alloc();
    if ( !inputFrame || !filterFrame )
        Fatal( "Can't allocate filter frames" );

    if ( waitForProviders() )
    {
        uint16_t inputWidth = videoProvider()->width();
        uint16_t inputHeight = videoProvider()->height();
        PixelFormat inputPixelFormat = videoProvider()->pixelFormat();
        AVRational timeBase = videoProvider()->frameRate().timeBase();

        char args[512];
        AVFilter *filterBufferSource  = avfilter_get_by_name( "buffer" );
        AVFilter *filterBufferSink = avfilter_get_by_name( "buffersink" );
        AVFilterInOut *outputs = avfilter_inout_alloc();
        AVFilterInOut *inputs  = avfilter_inout_alloc();
        enum AVPixelFormat pix_fmts[] = { inputPixelFormat, AV_PIX_FMT_NONE };
        mFilterGraph = avfilter_graph_alloc();
        if ( !outputs || !inputs || !mFilterGraph )
            Fatal( "Can't allocate filter memory" );

        /* buffer video source: the decoded frames from the decoder will be inserted here. */
        //snprintf( args, sizeof(args),
                //"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:inputPixel_aspect=%d/%d",
                //inputWidth, inputHeight, inputPixelFormat,
                //timeBase.num, timeBase.den,
                //dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den );
        snprintf( args, sizeof(args),
                "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
                inputWidth, inputHeight, inputPixelFormat,
                timeBase.num, timeBase.den );
        if ( avfilter_graph_create_filter( &mBufferSrcContext, filterBufferSource, "in", args, NULL, mFilterGraph ) < 0 )
            Fatal( "Can't create filter buffer source '%s'", args );

        /* buffer video sink: to terminate the filter chain. */
        if ( avfilter_graph_create_filter( &mBufferSinkContext, filterBufferSink, "out", NULL, NULL, mFilterGraph ) < 0 )
            Fatal( "Can't create filter buffer sink '%s'", args );

        if ( av_opt_set_int_list( mBufferSinkContext, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN ) < 0 )
            Fatal( "Can't set filter output pixel format" );

        /*
        * Set the endpoints for the filter graph. The mFilterGraph will
        * be linked to the graph described by filters_descr.
        */
        /*
        * The buffer source output must be connected to the input pad of
        * the first filter described by filters_descr; since the first
        * filter input label is not specified, it is set to "in" by
        * default.
        */
        outputs->name       = av_strdup("in");
        outputs->filter_ctx = mBufferSrcContext;
        outputs->pad_idx    = 0;
        outputs->next       = NULL;
        /*
        * The buffer sink input must be connected to the output pad of
        * the last filter described by filters_descr; since the last
        * filter output label is not specified, it is set to "out" by
        * default.
        */
        inputs->name       = av_strdup("out");
        inputs->filter_ctx = mBufferSinkContext;
        inputs->pad_idx    = 0;
        inputs->next       = NULL;
        if ( avfilter_graph_parse_ptr( mFilterGraph, mFilter.c_str(), &inputs, &outputs, NULL ) < 0)
            Fatal( "Unable to parse filter" );
        if ( avfilter_graph_config( mFilterGraph, NULL ) < 0)
            Fatal( "Unable to config filter" );

        avfilter_inout_free( &inputs );
        avfilter_inout_free( &outputs );

        Debug( 1,"Applying filter '%s'", mFilter.c_str() );

        int outputWidth = width();
        int outputHeight = height();
        PixelFormat outputPixelFormat = pixelFormat();
        ByteBuffer outputBuffer;

        Debug(1, "Filtering from %d x %d @ %d -> %d x %d @ %d", inputWidth, inputHeight, inputPixelFormat, outputWidth, outputHeight, outputPixelFormat );

        // Make space for anything that is going to be output
        outputBuffer.size( avpicture_get_size( outputPixelFormat, outputWidth, outputHeight ) );

        while ( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                Debug( 3, "Got %zd frames on queue", mFrameQueue.size() );
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    const VideoFrame *frame = dynamic_cast<const VideoFrame *>(iter->get());

                    if ( !frame )
                        continue;

                    inputFrame->width = inputWidth;
                    inputFrame->height = inputHeight;
                    inputFrame->format = inputPixelFormat;
                    inputFrame->pts = frame->timestamp();
                    avpicture_fill( (AVPicture *)inputFrame, frame->buffer().data(), inputPixelFormat, inputWidth, inputHeight );

                    if ( av_buffersrc_add_frame( mBufferSrcContext, inputFrame ) < 0 )
                    {
                        Error( "Error while feeding the filtergraph" );
                        break;
                    }
                    while ( true )
                    {
                        int result = av_buffersink_get_frame( mBufferSinkContext, filterFrame );
                        if ( result == AVERROR(EAGAIN) || result == AVERROR_EOF )
                        {
                            break; // Done?
                        }
                        if ( result < 0 )
                        {
                            Error( "Error while extracting from filtergraph" );
                            mStop = true;
                            break;
                        }
                        avpicture_layout( (const AVPicture *)filterFrame, outputPixelFormat, outputWidth, outputHeight, outputBuffer.data(), outputBuffer.size() );
                        VideoFrame *videoFrame = new VideoFrame( this, *iter, mFrameCount, frame->timestamp(), outputBuffer );
                        distributeFrame( FramePtr( videoFrame ) );
                        av_frame_unref( filterFrame );
                    }
                    mFrameCount++;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();

            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
        FeedProvider::cleanup();
        FeedConsumer::cleanup();
        avfilter_graph_free( &mFilterGraph );
    }
    av_frame_free( &inputFrame );
    av_frame_free( &filterFrame );
    return( !ended() );
}

#include "../base/oz.h"
#include "ozDummyInput.h"

#include "../base/ozFfmpeg.h"
#include "../libgen/libgenDebug.h"

DummyInput::DummyInput( const std::string &name, const Options &options ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mOptions( options )
    //mWidth( 0 ),
    //mHeight( 0 ),
    //mPixelFormat( PIX_FMT_NONE ),
    //mFrameRate( 0 ),
    //mColour( "black" ),
    //mTestSize( 0 ),
    //mTextColour( "white" )
{
    mWidth = mOptions.get( "width", 1920 );
    mHeight = mOptions.get( "height", 1080 );
    mPixelFormat = mOptions.get( "pixelFormat", (AVPixelFormat)PIX_FMT_YUV420P );
    mFrameRate = mOptions.get( "frameRate", 10 );
    mColour = mOptions.get( "colour", "gray" );
    mText = mOptions.get( "text", "'%{localtime\\:%H\\\\\\:%M\\\\\\:%S}'" );
    mTextSize = mOptions.get( "textSize", 48 );
    mTextColour = mOptions.get( "textColour", "white" );
}

/**
* @brief 
*/
DummyInput::~DummyInput()
{
}

/**
* @brief 
*
* @return 
*/
int DummyInput::run()
{
    AVFilterContext *bufferSrcContext;
    AVFilterContext *bufferSinkContext;
    AVFilterGraph *filterGraph;

    AVFrame *outputFrame = av_frame_alloc();
    if ( !outputFrame )
        Fatal( "Can't allocate filter frames" );

    char args[512] = "";
    AVFilter *filterBufferSource = avfilter_get_by_name( "color" );
    AVFilter *filterBufferSink = avfilter_get_by_name( "buffersink" );
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = { mPixelFormat, AV_PIX_FMT_NONE };
    filterGraph = avfilter_graph_alloc();
    if ( !outputs || !inputs || !filterGraph )
        Fatal( "Can't allocate filter memory" );

    snprintf( args, sizeof(args),
            "c=%s:size=%dx%d:r=%d/%d",
            mColour.c_str(), mWidth, mHeight, mFrameRate.timeBase().den, mFrameRate.timeBase().num );
    if ( avfilter_graph_create_filter( &bufferSrcContext, filterBufferSource, "in", args, NULL, filterGraph ) < 0 )
        Fatal( "Can't create filter buffer source '%s'", args );

    /* buffer video sink: to terminate the filter chain. */
    if ( avfilter_graph_create_filter( &bufferSinkContext, filterBufferSink, "out", NULL, NULL, filterGraph ) < 0 )
        Fatal( "Can't create filter buffer sink '%s'", args );

    if ( av_opt_set_int_list( bufferSinkContext, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN ) < 0 )
        Fatal( "Can't set filter output pixel format" );

    /*
    * Set the endpoints for the filter graph. The filterGraph will
    * be linked to the graph described by filters_descr.
    */
    /*
    * The buffer source output must be connected to the input pad of
    * the first filter described by filters_descr; since the first
    * filter input label is not specified, it is set to "in" by
    * default.
    */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = bufferSrcContext;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;
    /*
    * The buffer sink input must be connected to the output pad of
    * the last filter described by filters_descr; since the last
    * filter output label is not specified, it is set to "out" by
    * default.
    */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = bufferSinkContext;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    std::string filterString;
    if ( mText.length() )
        filterString = "drawtext=fontsize="+std::to_string(mTextSize)+":fontcolor="+mTextColour+":fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:text="+mText+":x=(w-text_w)/2:y=(h-text_h)/2";
    else
        filterString = "copy";
    Info( "Filter:%s", filterString.c_str() );

    if ( avfilter_graph_parse_ptr( filterGraph, filterString.c_str(), &inputs, &outputs, NULL ) < 0)
        Fatal( "Unable to parse filter" );
    if ( avfilter_graph_config( filterGraph, NULL ) < 0)
        Fatal( "Unable to config filter" );

    avfilter_inout_free( &inputs );
    avfilter_inout_free( &outputs );

    Debug( 1, "Creating generator filter '%s'", args );

    int numInputs = bufferSinkContext->nb_inputs;
    const AVFilterLink *filterLink = bufferSinkContext->inputs[numInputs-1];
    if ( !filterLink )
        throw Exception( "No buffer sink input" );

    int outputWidth = filterLink->w;
    int outputHeight = filterLink->h;
    PixelFormat outputPixelFormat = (PixelFormat)filterLink->format;
    FrameRate outputFrameRate = filterLink->frame_rate;

    ByteBuffer outputBuffer;

    // Make space for anything that is going to be output
    outputBuffer.size( avpicture_get_size( outputPixelFormat, outputWidth, outputHeight ) );

    uint64_t timeInterval = mFrameRate.intervalUsec();
    uint64_t currTime = time64();
    uint64_t nextTime = currTime;
    while ( !mStop )
    {
        while ( true )
        {
            // Synchronise the output with the desired output frame rate
            while ( currTime < nextTime )
            {
                currTime = time64();
                usleep( INTERFRAME_TIMEOUT );
            }
            nextTime += timeInterval;

            int result = av_buffersink_get_frame( bufferSinkContext, outputFrame );
            if ( result == AVERROR(EAGAIN) || result == AVERROR_EOF )
                break; // Done?

            if ( result < 0 )
            {
                Error( "Error while extracting from filtergraph" );
                mStop = true;
                break;
            }
            avpicture_layout( (const AVPicture *)outputFrame, outputPixelFormat, outputWidth, outputHeight, outputBuffer.data(), outputBuffer.size() );
            VideoFrame *videoFrame = new VideoFrame( this, mFrameCount, currTime, outputBuffer );
            distributeFrame( FramePtr( videoFrame ) );
            av_frame_unref( outputFrame );
            mFrameCount++;
        }
    }
    avfilter_graph_free( &filterGraph );
    av_frame_free( &outputFrame );
    cleanup();
    return( !ended() );
}

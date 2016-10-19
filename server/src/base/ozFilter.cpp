#include "oz.h"
#include "ozFilter.h"

#include "ozFfmpeg.h"

/**
* @brief 
*
* @param pixelFormat
* @param width
* @param height
* @param provider
* @param link
*/
ImageFilter::ImageFilter( const std::string &filter, uint16_t width, uint16_t height, PixelFormat pixelFormat ) :
    mFilter( filter ),
    mInputWidth( width ),
    mInputHeight( height ),
    mInputPixelFormat( pixelFormat ),
    mBufferSrcContext( NULL ),
    mBufferSinkContext( NULL ),
    mFilterGraph( NULL )
{
    mFilterBufferSource = avfilter_get_by_name( "buffer" );
    mFilterBufferSink = avfilter_get_by_name( "buffersink" );

    mFilterOutputs = avfilter_inout_alloc();
    mFilterInputs  = avfilter_inout_alloc();

    mFilterGraph = avfilter_graph_alloc();
    if ( !mFilterOutputs || !mFilterInputs || !mFilterGraph )
        Fatal( "Can't allocate filter memory" );

    mInputFrame = av_frame_alloc();
    mFilterFrame = av_frame_alloc();
    if ( !mInputFrame || !mFilterFrame )
        Fatal( "Can't allocate filter frames" );

    char args[512];
    snprintf( args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=1/25",
            mInputWidth, mInputHeight, mInputPixelFormat );
    if ( avfilter_graph_create_filter( &mBufferSrcContext, mFilterBufferSource, "in", args, NULL, mFilterGraph ) < 0 )
        Fatal( "Can't create filter buffer source '%s'", args );
    mSignature = mFilter+":"+args;

    /* buffer video sink: to terminate the filter chain. */
    if ( avfilter_graph_create_filter( &mBufferSinkContext, mFilterBufferSink, "out", NULL, NULL, mFilterGraph ) < 0 )
        Fatal( "Can't create filter buffer sink '%s'", args );

    enum AVPixelFormat pix_fmts[] = { mInputPixelFormat, AV_PIX_FMT_NONE };
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
    mFilterOutputs->name       = av_strdup("in");
    mFilterOutputs->filter_ctx = mBufferSrcContext;
    mFilterOutputs->pad_idx    = 0;
    mFilterOutputs->next       = NULL;
    /*
    * The buffer sink input must be connected to the output pad of
    * the last filter described by filters_descr; since the last
    * filter output label is not specified, it is set to "out" by
    * default.
    */
    mFilterInputs->name       = av_strdup("out");
    mFilterInputs->filter_ctx = mBufferSinkContext;
    mFilterInputs->pad_idx    = 0;
    mFilterInputs->next       = NULL;
    if ( avfilter_graph_parse_ptr( mFilterGraph, mFilter.c_str(), &mFilterInputs, &mFilterOutputs, NULL ) < 0)
        Fatal( "Unable to parse filter" );
    if ( avfilter_graph_config( mFilterGraph, NULL ) < 0)
        Fatal( "Unable to config filter" );

    avfilter_inout_free( &mFilterInputs );
    avfilter_inout_free( &mFilterOutputs );

    int numInputs = mBufferSinkContext->nb_inputs;
    const AVFilterLink *lastFilterLink = mBufferSinkContext->inputs[numInputs-1];
    if ( !lastFilterLink )
        Fatal( "No buffer sink input" );

    Debug( 1,"Creating filter '%s'", mSignature.c_str() );

    mOutputWidth = lastFilterLink->w;
    mOutputHeight = lastFilterLink->h;
    mOutputPixelFormat = (PixelFormat)lastFilterLink->format;

    Debug( 1, "Filtering from %d x %d @ %d -> %d x %d @ %d", mInputWidth, mInputHeight, mInputPixelFormat, mOutputWidth, mOutputHeight, mOutputPixelFormat );

    // Make space for anything that is going to be output
    mOutputBuffer.size( avpicture_get_size( mOutputPixelFormat, mOutputWidth, mOutputHeight ) );

    mInputFrame->width = mInputWidth;
    mInputFrame->height = mInputHeight;
    mInputFrame->format = mInputPixelFormat;
}

ImageFilter::ImageFilter( const std::string &filter, const Image &image ) : 
    ImageFilter( filter, image.width(), image.height(), image.pixelFormat() )
{
}

/**
* @brief 
*/
ImageFilter::~ImageFilter()
{
    avfilter_graph_free( &mFilterGraph );
    av_frame_free( &mInputFrame );
    av_frame_free( &mFilterFrame );

    mBufferSrcContext = NULL;
    mBufferSinkContext = NULL;
    mFilterGraph = NULL;
}

/**
* @brief 
*
* @return 
*/
Image *ImageFilter::execute( const Image &inputImage )
{
    if ( inputImage.width() != mInputWidth || inputImage.height() != mInputHeight || inputImage.pixelFormat() != mInputPixelFormat )
        Fatal( "Invalid image %d x %d @ %d sent to filter, expecting %d x %d @ %d", 
            inputImage.width(), inputImage.height(), inputImage.pixelFormat(),
            mInputWidth, mInputHeight, mInputPixelFormat
        );

    avpicture_fill( (AVPicture *)mInputFrame, inputImage.buffer().data(), mInputPixelFormat, mInputWidth, mInputHeight );

    if ( av_buffersrc_add_frame( mBufferSrcContext, mInputFrame ) < 0 )
    {
        Error( "Error while feeding the filtergraph" );
        return( NULL );
    }

    Image *outputImage = NULL;
    while ( true )
    {
        int result = av_buffersink_get_frame( mBufferSinkContext, mFilterFrame );
        if ( result == AVERROR(EAGAIN) || result == AVERROR_EOF )
            break; // Done?
        if ( result < 0 )
            Fatal( "Error while extracting from filtergraph" );
        avpicture_layout( (const AVPicture *)mFilterFrame, mOutputPixelFormat, mOutputWidth, mOutputHeight, mOutputBuffer.data(), mOutputBuffer.size() );
        if ( outputImage )
            delete outputImage;
        outputImage = new Image( mOutputPixelFormat, mOutputWidth, mOutputHeight, mOutputBuffer );
        av_frame_unref( mFilterFrame );
    }
    return( outputImage );
}

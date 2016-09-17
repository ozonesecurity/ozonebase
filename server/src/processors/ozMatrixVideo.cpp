#include "../base/oz.h"
#include "ozMatrixVideo.h"

#include "../base/ozFeedFrame.h"

/**
* @brief 
*
* @param name
* @param pixelFormat
* @param width
* @param height
* @param frameRate
* @param xTiles
* @param yTiles
*/
MatrixVideo::MatrixVideo( const std::string &name, PixelFormat pixelFormat, int width, int height, FrameRate frameRate, int xTiles, int yTiles ) :
    VideoConsumer( cClass(), name, xTiles*yTiles ),
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mPixelFormat( pixelFormat ),
    mWidth( width ),
    mHeight( height ),
    mFrameRate( frameRate ),
    mProviderList( mProviderLimit ),
    mTilesX( xTiles ),
    mTilesY( yTiles ),
    mTiles( mTilesX * mTilesY )
{
    for ( int i = 0; i < mProviderLimit; i++ )
        mProviderList[i] = NULL;
    mInterFrames = new AVFrame *[mTiles];
    memset( mInterFrames, 0, sizeof(*mInterFrames)*mTiles );
    mConvertContexts = new struct SwsContext *[mTiles];
    memset( mConvertContexts, 0, sizeof(*mConvertContexts)*mTiles );
}

/**
* @brief 
*/
MatrixVideo::~MatrixVideo()
{
    delete[] mConvertContexts;
    delete[] mInterFrames;
}

/**
* @brief 
*
* @param provider
* @param link
*
* @return 
*/
bool MatrixVideo::registerProvider( FeedProvider &provider, const FeedLink &link )
{
    for ( int i = 0; i < mProviderList.size(); i++ )
    {
        if ( !mProviderList[i] )
        {
            if ( VideoConsumer::registerProvider( provider, link ) )
            {
                mProviderList[i] = &provider;
                return( true );
            }
        }
    }
    return( false );
}

/**
* @brief 
*
* @param provider
* @param reciprocate
*
* @return 
*/
bool MatrixVideo::deregisterProvider( FeedProvider &provider, bool reciprocate )
{
    for ( int i = 0; i < mProviderList.size(); i++ )
    {
        if ( mProviderList[i] == &provider )
        {
            mProviderList[i] = NULL;
            if ( mConvertContexts[i] )
                sws_freeContext( mConvertContexts[i] );
            mConvertContexts[i] = NULL;
            if ( mInterFrames[i] )
                av_free( mInterFrames[i] );
            mInterFrames[i] = NULL;
            break;
        }
    }
    return( VideoConsumer::deregisterProvider( provider, reciprocate ) );
}

/**
* @brief 
*
* @return 
*/
int MatrixVideo::run()
{
    // Make sure ffmpeg is compiled with mpjpeg support
    AVCodec *codec = avcodec_find_encoder( CODEC_ID_MJPEG );
    if ( !codec )
        Fatal( "Can't find encoder codec" );

    Debug( 2, "Time base = %d/%d", mFrameRate.num, mFrameRate.den );
    Debug( 2, "Pix fmt = %d", mPixelFormat );

    AVFrame *inputFrame = avcodec_alloc_frame();
    AVFrame *outputFrame = avcodec_alloc_frame();

    // Wait for video source to be ready
    if ( waitForProviders() )
    {
        setReady();

        // Make space for anything that is going to be output
        ByteBuffer outputBuffer;
        outputBuffer.size( avpicture_get_size( mPixelFormat, mWidth, mHeight ) );

        // To get offsets only
        avpicture_fill( (AVPicture *)outputFrame, outputBuffer.data(), mPixelFormat, mWidth, mHeight );

        double timeInterval = (double)mFrameRate.num/mFrameRate.den;
        struct timeval now;
        gettimeofday( &now, 0 );
        long double currTime = now.tv_sec+((double)now.tv_usec/1000000.0);
        long double nextTime = currTime;
        uint16_t interWidth = mWidth/mTilesX;    // Per tile
        uint16_t interHeight = mHeight/mTilesY;
        const AVPixFmtDescriptor &interFormatDesc = av_pix_fmt_descriptors[mPixelFormat];
        while ( !mStop )
        {
            // Synchronise the output with the desired output frame rate
            while ( currTime < nextTime )
            {
                gettimeofday( &now, 0 );
                currTime = now.tv_sec+((double)now.tv_usec/1000000.0);
                usleep( INTERFRAME_TIMEOUT );
            }
            nextTime += timeInterval;

            if ( hasConsumer() )
            {
                for ( int i = 0; i < mProviderList.size(); i++ )
                {
                    FeedProvider *provider = mProviderList[i];
                    if ( provider )
                    {
                        Debug( 4, "Got provider %d", i );
                        FramePtr framePtr = provider->pollFrame();
                        const FeedFrame *frame = framePtr.get();

                        if ( frame )
                        {
                            const VideoFrame *inputVideoFrame = dynamic_cast<const VideoFrame *>(frame);
                            Debug( 2,"PF:%d @ %dx%d", inputVideoFrame->pixelFormat(), inputVideoFrame->width(), inputVideoFrame->height() );
                            const VideoProvider *videoProvider = inputVideoFrame->videoProvider();

                            if ( !mInterFrames[i] )
                            {
                                AVFrame *interFrame = mInterFrames[i] = avcodec_alloc_frame();
                                avpicture_fill( (AVPicture *)interFrame, outputBuffer.data(), mPixelFormat, interWidth, interHeight );

                                int x = i % mTilesX;
                                int y = i / mTilesX;
                                // Now adjust the data pointers
                                for ( int plane = 0; plane < AV_NUM_DATA_POINTERS; plane++ )
                                {
                                    if ( interFrame->linesize[plane] )
                                    {
                                        int planeHeight;
                                        //int planeWidth;
                                        if ( plane == 0 )
                                        {
                                            //planeWidth = interWidth;
                                            planeHeight = interHeight;
                                        }
                                        else
                                        {
                                            //planeWidth = -((-interWidth ) >> interFormatDesc.log2_chroma_w);
                                            planeHeight = -((-interHeight ) >> interFormatDesc.log2_chroma_h);
                                        }
                                        Debug( 5, "Moving image %d, plane %d", i, plane );

                                        uint8_t *address = outputFrame->data[plane];
                                        Debug( 5, "address1: %p", address );
                                        address += (y * planeHeight * outputFrame->linesize[plane]);
                                        Debug( 5, "address2: %p (y:%d * ph:%d * ols:%d)", address, y, planeHeight, outputFrame->linesize[plane] );
                                        address += x * interFrame->linesize[plane];
                                        Debug( 5, "address3: %p (x:%d * ils:%d)", address, x, interFrame->linesize[plane] );
                                        Debug( 5, "Moving i:%d, x:%d, y:%d, linesize: %d", i, x, y, interFrame->linesize[plane] );
                                        interFrame->data[plane] = address;
                                        interFrame->linesize[plane] *= mTilesX;
                                    }
                                }
                            }

                            uint16_t inputWidth = videoProvider->width();
                            uint16_t inputHeight = videoProvider->height();
                            PixelFormat inputPixelFormat = videoProvider->pixelFormat();

                            if ( !mConvertContexts[i] )
                            {
                                // Prepare for image format and size conversions
                                struct SwsContext *convertContext =  mConvertContexts[i] = sws_getContext( inputWidth, inputHeight, inputPixelFormat, interWidth, interHeight, mPixelFormat, SWS_BICUBIC, NULL, NULL, NULL );
                                if ( !convertContext )
                                    Fatal( "Unable to create conversion context for provider %s", videoProvider->cidentity() );
            
                                Debug( 1,"Converting from %d x %d @ %d -> %d x %d @ %d", inputWidth, inputHeight, inputPixelFormat, interWidth, interHeight, mPixelFormat );
                                Debug( 2, "%d bytes -> %d bytes",  avpicture_get_size( inputPixelFormat, inputWidth, inputHeight ), avpicture_get_size( mPixelFormat, interWidth, interHeight ) );
                            }
                            avpicture_fill( (AVPicture *)inputFrame, inputVideoFrame->buffer().data(), inputPixelFormat, inputWidth, inputHeight );

                            // Reformat the input frame to fit the desired output format
                            if ( sws_scale( mConvertContexts[i], inputFrame->data, inputFrame->linesize, 0, inputHeight, mInterFrames[i]->data, mInterFrames[i]->linesize ) < 0 )
                                Fatal( "Unable to convert input frame (%d@%dx%d) to output frame (%d@%dx%d) at frame %ju", inputPixelFormat, inputWidth, inputHeight, mPixelFormat, interWidth, interHeight, mFrameCount );
                        }
                    }
                }
                Debug( 3, "Queueing frame %ju", mFrameCount );
                VideoFrame *outputVideoFrame = new VideoFrame( this, ++mFrameCount, currTime*1000000LL, outputBuffer );
                distributeFrame( FramePtr( outputVideoFrame ) );
            }

            // Clear queue to prevent it filling up, proper polled interface should not use queues
            mQueueMutex.lock();
            mFrameQueue.clear();
            mQueueMutex.unlock();
            checkProviders();
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    for ( int i = 0; i < mProviderList.size(); i++ )
    {
        if ( mConvertContexts[i] )
            sws_freeContext( mConvertContexts[i] );
        if ( mInterFrames[i] )
            av_free( mInterFrames[i] );
    }
    av_free( outputFrame );
    av_free( inputFrame );
    return( !ended() );
}

#if 0 // Filter version
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/vsrc_buffer.h>
#include <libavfilter/buffersink.h>

int MatrixVideo::run()
{
    avfilter_register_all();

    AVFilterContext *colourSrcContext;
    AVFilterContext *bufferSrcContexts[4] = { NULL };
    AVFilterContext *bufferSinkContext;

    AVFilter *colourSrc  = avfilter_get_by_name( "color" );
    AVFilter *bufferSrc  = avfilter_get_by_name( "buffersrc" );
    AVFilter *bufferSink = avfilter_get_by_name( "buffersink" );
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    enum PixelFormat pixelFormats[] = { mPixelFormat };
    AVFilterGraph *filterGraph = avfilter_graph_alloc();

    /* color video source: default background colour. */
    std::string inputArgs = stringtf( "red:%dx%d:0.0", mWidth, mHeight );
    if ( avfilter_graph_create_filter( &colourSrcContext, colourSrc, "in", inputArgs.c_str(), NULL, filterGraph ) )
        Fatal( "Cannot create colour source" );

    /* buffer video sink: to terminate the filter chain. */
    if ( avfilter_graph_create_filter( &bufferSinkContext, bufferSink, "out", NULL, pixelFormats, filterGraph ) )
        Fatal( "Cannot create buffer sink" );

    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = colourSrcContext;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = bufferSinkContext;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    const char *filter_desc = "scale=78:24";
    if ( avfilter_graph_parse( filterGraph, filter_desc, &inputs, &outputs, NULL ) < 0 )
        Fatal( "Can't parse filter graph '%s'", filter_desc );

    if ( avfilter_graph_config( filterGraph, NULL ) < 0 )
        Fatal( "Can't configure filter graph" );

    if ( !waitForProviders() )
    {
        setEnded();
        return( -1 );
    }

    AVFrame avInputFrame;
    size_t avInputFrameSize = avpicture_get_size( mPixelFormat, mWidth, mHeight ); // Just a guess at this point
    ByteBuffer avInputFrameBuffer( avInputFrameSize );

    // Make space for anything that is going to be output
    AVFrame avOutputFrame;
    ByteBuffer avOutputBuffer;
    avOutputBuffer.size( avpicture_get_size( mPixelFormat, mWidth, mHeight ) );
    avpicture_fill( (AVPicture *)&avOutputFrame, avOutputBuffer.data(), mPixelFormat, mWidth, mHeight );
                     
    while ( !mStop )
    {
        mQueueMutex.lock();
        if ( !mFrameQueue.empty() )
        {
            Debug( 3, "Got %zd frames on queue", mFrameQueue.size() );
            for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
            {
                const VideoFrame *inputFrame = dynamic_cast<const VideoFrame *>(iter->get());

                Debug(1, "%s / Provider: %s, Source: %s, Frame: %p (%ju / %.3f) - %d", cid(), inputFrame->provider()->cidentity(), inputFrame->originator()->cidentity(), inputFrame, inputFrame->id(), inputFrame->age(), inputFrame->buffer().size() );

                avInputFrameSize = avpicture_get_size( inputFrame->pixelFormat(), inputFrame->width(), inputFrame->height() );
                avcodec_get_frame_defaults( &avInputFrame );
                avInputFrame.pts = inputFrame->timestamp();
                avpicture_fill( (AVPicture *)&avInputFrame, inputFrame->buffer().data(), inputFrame->pixelFormat(), inputFrame->width(), inputFrame->height() );
                            
                ProviderIndices::iterator iter = mProviderIndices.find( inputFrame->provider() );
                if ( iter == mProviderIndices.end() )
                    Fatal( "Unable to find index of provider %s", inputFrame->provider()->cidentity() );
                int providerIndex = iter->second;

                if ( !bufferSrcContexts[providerIndex] )
                {
                    std::string inputArgs = stringtf( "%d:%d:%d:%d:%d:1:1",
                        inputFrame->width(),
                        inputFrame->height(),
                        inputFrame->pixelFormat(),
                        inputFrame->videoProvider()->frameRate().num,
                        inputFrame->videoProvider()->frameRate().den
                    );
                    if ( avfilter_graph_create_filter( &bufferSrcContexts[providerIndex], bufferSrc, "in", inputArgs.c_str(), NULL, filterGraph ) < 0 )
                        Fatal( "Cannot create buffer source %d for provider %s", providerIndex, inputFrame->provider()->cidentity() );
                }

                AVFilterBufferRef *picref;

                /* push the decoded frame into the filtergraph */
                av_vsrc_buffer_add_frame( bufferSrcContexts[providerIndex], &avInputFrame, AV_VSRC_BUF_FLAG_OVERWRITE );

                /* pull filtered pictures from the filtergraph */
                while ( avfilter_poll_frame(bufferSinkContext->inputs[0]) )
                {
                    av_buffersink_get_buffer_ref( bufferSinkContext, &picref, 0 );
                    if ( picref )
                    {
                        VideoFrame *videoFrame = new VideoFrame( this, mFrameCount, picref->pts, avOutputBuffer );
                        distributeFrame( FramePtr( videoFrame ) );
                        mFrameCount++;
                        //display_picref(picref, bufferSinkContext->inputs[0]->time_base);
                        avfilter_unref_buffer(picref);
                    }
                }
            }
            mFrameQueue.clear();
        }
        mQueueMutex.unlock();
        // Quite short so we can always keep up with the required packet rate for 25/30 fps
        usleep( INTERFRAME_TIMEOUT );
    }

    avfilter_graph_free( &filterGraph );

    return( 0 );
}
#endif

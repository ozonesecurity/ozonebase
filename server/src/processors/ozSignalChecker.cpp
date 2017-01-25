#include "../base/oz.h"
#include "ozSignalChecker.h"

#include "../base/ozFeedFrame.h"
#include <sys/time.h>

/**
* @brief 
*
* @param name
*/
SignalChecker::SignalChecker( const std::string &name ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mSignal( false )
{
}

/**
* @brief 
*/
SignalChecker::~SignalChecker()
{
    mSignal = false;
}

/**
* @brief 
*
* @return 
*/
int SignalChecker::run()
{
    if ( waitForProviders() )
    {
        PixelFormat producerPixelFormat = videoProvider()->pixelFormat();
        uint16_t producerWidth = videoProvider()->width();
        uint16_t producerHeight = videoProvider()->height();

        bool lastSignal = false;
        while ( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                Debug( 3, "Got %zd frames on queue", mFrameQueue.size() );
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    const VideoFrame *frame = dynamic_cast<const VideoFrame *>(iter->get());

                    Image image( producerPixelFormat, producerWidth, producerHeight, frame->buffer().data() );

                    mSignal = checkSignal( image );
                    if ( mSignal && !lastSignal )
                    {
                        Info( "Signal Reacquired @ %ju", mFrameCount );
                    }
                    else if ( !mSignal && lastSignal )
                    {
                        Info( "Signal Lost @ %ju", mFrameCount );
                    }

                    // Can probably just distribute original frame, depends if we want trail of processors
                    VideoFrame *videoFrame = new VideoFrame( this, *iter );
                    distributeFrame( FramePtr( videoFrame ) );

                    //if ( mSignal )
                    //{
                        //distributeFrame( frame );
                    //}

                    //delete *iter;
                    lastSignal = mSignal;
                    mFrameCount++;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}

/**
* @brief 
*
* @param image
*
* @return 
*/
bool SignalChecker::checkSignal( const Image &image )
{
    uint32_t signalCheckColour = 0x010203;

    // FIXME - Shouldn't be static
    static bool staticUndef = true;
    static unsigned char redVal;
    static unsigned char greenVal;
    static unsigned char blueVal;

    if ( config.signal_check_points > 0 )
    {
        if ( staticUndef )
        {
            staticUndef = false;
            redVal = RGB_RED_VAL(signalCheckColour);
            greenVal = RGB_GREEN_VAL(signalCheckColour);
            blueVal = RGB_BLUE_VAL(signalCheckColour);
        }

        int width = image.width();
        //int height = image.height();

        int x, y = 0;
        for ( int i = 0; i < config.signal_check_points; i++ )
        {
            x = (int)(((long long)rand()*(long long)(width-1))/RAND_MAX);
// XXX - Do as exclusion box
#if 0
            while( true )
            {
                if ( !config.timestamp_on_capture || !identity()Format[0] )
                    break;
                // Avoid sampling the rows with timestamp in
                y = (int)(((long long)rand()*(long long)(height-1))/RAND_MAX);
                if ( y < identity()Coord.y() || y >= identity()Coord.y()+Image::LINE_HEIGHT )
            }
#endif
            if ( (image.red(x,y) != redVal) || (image.green(x,y) != greenVal) || (image.blue(x,y) != blueVal) )
                return( true );
        }
        return( false );
    }
    return( true );
}

/**
* @brief 
*
* @param frame
* @param 
*
* @return 
*/
bool SignalChecker::signalValid( const FramePtr &frame, const FeedConsumer * )
{
    const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame.get());
    const SignalChecker *signalChecker = dynamic_cast<const SignalChecker *>( videoFrame->provider() );
    if ( !signalChecker )
        Panic( "Can't check signal valid, frame source not signal checker" );
    Debug( 2,"Checking valid - %d", signalChecker->hasSignal() );
    return( signalChecker->hasSignal() );
}

/**
* @brief 
*
* @param frame
* @param 
*
* @return 
*/
bool SignalChecker::signalInvalid( const FramePtr &frame, const FeedConsumer * )
{
    const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame.get());
    const SignalChecker *signalChecker = dynamic_cast<const SignalChecker *>( videoFrame->provider() );
    if ( !signalChecker )
        Panic( "Can't check signal valid, frame source not signal checker" );
    Info( "Checking invalid - %d", !signalChecker->hasSignal() );
    return( !signalChecker->hasSignal() );
}

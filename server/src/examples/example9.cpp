#include "../base/ozApp.h"
#include "../providers/ozAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../consumers/ozVideoRecorder.h"
#include "../consumers/ozMovieFileOutput.h"
#include "../base/ozNotifyFrame.h"

#include "../libgen/libgenDebug.h"

class NotifyOutput : public DataConsumer, public Thread
{
CLASSID(NotifyOutput);

protected:
    int run();
    bool processFrame( FramePtr );

public:
    NotifyOutput( const std::string &name ) :
        DataConsumer( cClass(), name, 8 ),
        Thread( identity() )
    {
    }
};

int NotifyOutput::run()
{
    if ( waitForProviders() )
    {
        while( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    processFrame( *iter );
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    cleanup();
    return( 0 );
}

bool NotifyOutput::processFrame( FramePtr frame )
{
    Info( "Got frame" );

    const NotifyFrame *notifyFrame = dynamic_cast<const NotifyFrame *>(frame.get());

    // Anything else we ignore
    if ( notifyFrame )
    {
        const DiskIONotification *diskNotifyFrame = dynamic_cast<const DiskIONotification *>(notifyFrame);
        if ( diskNotifyFrame )
        {
            printf( "Got disk I/O notification frame\n" );
            const DiskIONotification::DiskIODetail &detail = diskNotifyFrame->detail();
            std::string typeString;
            switch( detail.type() )
            {
                case DiskIONotification::DiskIODetail::READ :
                {
                    typeString = "read";
                    break;
                }
                case DiskIONotification::DiskIODetail::WRITE :
                {
                    typeString = "write";
                    break;
                }
                case DiskIONotification::DiskIODetail::APPEND :
                {
                    typeString = "append";
                    break;
                }
            }
            switch ( detail.stage() )
            {
                case DiskIONotification::DiskIODetail::BEGIN :
                {
                    printf( "  DiskIO %s starting for file %s\n", typeString.c_str(), detail.name().c_str() );
                    break;
                }
                case DiskIONotification::DiskIODetail::PROGRESS :
                {
                    printf( "  DiskIO %s in progress for file %s\n", typeString.c_str(), detail.name().c_str() );
                    break;
                }
                case DiskIONotification::DiskIODetail::COMPLETE :
                {
                    printf( "  DiskIO %s completed for file %s, %ju bytes, result %d\n", typeString.c_str(), detail.name().c_str(), detail.size(), detail.result() );
                    break;
                }
            }
        }
        const EventNotification *eventNotifyFrame = dynamic_cast<const EventNotification *>(notifyFrame);
        if ( eventNotifyFrame )
        {
            printf( "Got event notification frame\n" );
            const EventNotification::EventDetail &detail = eventNotifyFrame->detail();
            switch ( detail.stage() )
            {
                case EventNotification::EventDetail::BEGIN :
                {
                    printf( "  Event %ju starting\n", detail.id() );
                    break;
                }
                case EventNotification::EventDetail::PROGRESS :
                {
                    printf( "  Event %ju in progress\n", detail.id() );
                    break;
                }
                case EventNotification::EventDetail::COMPLETE :
                {
                    printf( "  Event %ju completed, %.2lf seconds long\n", detail.id(), detail.length() );
                    break;
                }
            }
        }
    }
    return( true );
}

//
// Load images from network stream, run motion detection, write to MP4 files and trap and print notifications
// WARNING - This example can write lots of files, some large, to /tmp (or wherever configured)
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example9", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    AVInput input( "input", "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
    app.addThread( &input );

    MotionDetector motionDetector( "detector" );
    motionDetector.registerProvider( input );
    app.addThread( &motionDetector );

    VideoParms videoParms( 640, 480 );
    AudioParms audioParms;
    VideoRecorder recorder( "recorder" , "/transfer", "mp4", videoParms, audioParms );
    //MovieFileOutput recorder( "recorder" , "/tmp", "mp4", 300, videoParms, audioParms );
    recorder.registerProvider( motionDetector );
    app.addThread( &recorder );

    NotifyOutput notifier( "notifier" );
    notifier.registerProvider( recorder );
    app.addThread( &notifier );

    app.run();
}

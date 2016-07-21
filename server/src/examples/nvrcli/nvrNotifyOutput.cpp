#include "nvrNotifyOutput.h"

int NotifyOutput::run()
{

    while (1)
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
    }
    cleanup();
    return( 0 );
}

bool NotifyOutput::processFrame( FramePtr frame )
{

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

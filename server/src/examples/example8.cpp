#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozMemoryInputV1.h"
#include "../protocols/ozHttpController.h"
#include "../protocols/ozRtspController.h"

#include "../libgen/libgenDebug.h"

//
// Make images from ZM V1 shared memory available over the network
// You have to manually tune the shared mmemory sizes etc to match
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "ozx", "", 5 );

    Info( "Starting" );

    ffmpegInit();

    Application app;

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 28000, 28998 ) );
    listener.addController( &rtspController );

    HttpController httpController( "p8080", 8080 );
    listener.addController( &httpController );

    const int maxMonitors = 1;
    for ( int monitor = 1; monitor <= maxMonitors; monitor++ )
    {
        char idString[32] = "";

        // Get the individual images from shared memory
        sprintf( idString, "input%d", monitor );
        MemoryInputV1 *input = new MemoryInputV1( idString, "/dev/shm", monitor, 75, PIX_FMT_RGB24, 1920, 1080 );
        app.addThread( input );

        rtspController.addStream( "raw", *input );
        httpController.addStream( "raw", *input );
    }

    app.run();
}

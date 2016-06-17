#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozNetworkAVInput.h"
#include "../protocols/ozHttpController.h"
#include "../protocols/ozRtspController.h"

#include "../libgen/libgenDebug.h"

//
// Read video file and stream over Http and RTSP
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example4", "", 5 );

    Info( "Starting" );

    ffmpegInit();

    Application app;

    NetworkAVInput input( "input", "/tmp/movie.mp4" );
    app.addThread( &input );

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    HttpController httpController( "p8080", 8080 );

    rtspController.addStream( "raw", input );
    httpController.addStream( "raw", input );

    listener.addController( &rtspController );
    listener.addController( &httpController );
}

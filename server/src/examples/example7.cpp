#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozRawH264Input.h"
#include "../protocols/ozRtspController.h"

#include "../libgen/libgenDebug.h"

//
// Relay H.264 packets directly from source
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example7", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    RawH264Input input( "input", "rtsp://Admin:123456@10.10.10.10:7070" );
    app.addThread( &input );

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    listener.addController( &rtspController );

    rtspController.addStream( "raw", input );

    app.run();
}

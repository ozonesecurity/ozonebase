#include "../base/zmApp.h"
#include "../base/zmListener.h"
#include "../providers/zmLocalVideoInput.h"
#include "../providers/zmNetworkAVInput.h"
#include "../processors/zmQuadVideo.h"
#include "../protocols/zmRtspController.h"

#include "../libgen/libgenDebug.h"

//
// Load one file, one V4L and two network streams, combine to quad video available over RTSP
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example5", "", 5 );

    Info( "Starting" );

    ffmpegInit();

    Application app;

    LocalVideoInput input1( "input1", "/dev/video0" );
    app.addThread( &input1 );
    NetworkAVInput input2( "input1", "rtsp://test:test@webcam1/mpeg4/media.amp" );
    app.addThread( &input2 );
    NetworkAVInput input3( "input1", "rtsp://test:test@webcam2/mpeg4/media.amp" );
    app.addThread( &input3 );
    NetworkAVInput input4( "input", "/tmp/movie.mp4" );
    app.addThread( &input4 );

    QuadVideo quadVideo( "quad", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    quadVideo.registerProvider( input1 );
    quadVideo.registerProvider( input2 );
    quadVideo.registerProvider( input3 );
    quadVideo.registerProvider( input4 );
    app.addThread( &quadVideo );

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    listener.addController( &rtspController );

    rtspController.addStream( "input1", input1 );
    rtspController.addStream( "input2", input2 );
    rtspController.addStream( "input3", input3 );
    rtspController.addStream( "input4", input4 );
    rtspController.addStream( "quad", quadVideo );

    app.run();
}

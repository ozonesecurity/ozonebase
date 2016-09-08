#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozLocalVideoInput.h"
#include "../providers/ozAVInput.h"
#include "../processors/ozMatrixVideo.h"
#include "../protocols/ozRtspController.h"

#include "../libgen/libgenDebug.h"

//
// Load one file, one V4L and two network streams, combine to matrix video available over RTSP
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example5", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    LocalVideoInput input1( "input1", "/dev/video0" );
    app.addThread( &input1 );
    AVInput input2( "input1", "rtsp://test:test@webcam1/mpeg4/media.amp" );
    app.addThread( &input2 );
    AVInput input3( "input1", "rtsp://test:test@webcam2/mpeg4/media.amp" );
    app.addThread( &input3 );
    AVInput input4( "input", "/tmp/movie.mp4" );
    app.addThread( &input4 );

    MatrixVideo matrixVideo( "matrix", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    matrixVideo.registerProvider( input1 );
    matrixVideo.registerProvider( input2 );
    matrixVideo.registerProvider( input3 );
    matrixVideo.registerProvider( input4 );
    app.addThread( &matrixVideo );

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    listener.addController( &rtspController );

    rtspController.addStream( "input1", input1 );
    rtspController.addStream( "input2", input2 );
    rtspController.addStream( "input3", input3 );
    rtspController.addStream( "input4", input4 );
    rtspController.addStream( "matrix", matrixVideo );

    app.run();
}

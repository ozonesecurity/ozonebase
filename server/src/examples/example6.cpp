#include "../base/zmApp.h"
#include "../base/zmListener.h"
#include "../providers/zmNetworkAVInput.h"
#include "../processors/zmMotionDetector.h"
#include "../processors/zmMatrixVideo.h"
#include "../protocols/zmHttpController.h"

#include "../libgen/libgenDebug.h"

//
// Run motion detection on a saved file, provide debug images in matrix mode over HTTP
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example6", "", 5 );

    Info( "Starting" );

    ffmpegInit();

    Application app;

    NetworkAVInput input( "input", "/tmp/movie.mp4" );
    app.addThread( &input );

    MotionDetector motionDetector( "modect" );
    motionDetector.registerProvider( input );
    //EventRecorder eventRecorder( "/transfer/zmx" );
    app.addThread( &motionDetector );

    MatrixVideo matrixVideo( "matrix", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    matrixVideo.registerProvider( *motionDetector.refImageSlave() );
    matrixVideo.registerProvider( *motionDetector.compImageSlave() );
    matrixVideo.registerProvider( *motionDetector.deltaImageSlave() );
    matrixVideo.registerProvider( *motionDetector.varImageSlave() );
    app.addThread( &matrixVideo );

    Listener listener;
    app.addThread( &listener );

    HttpController httpController( "p8080", 8080 );
    listener.addController( &httpController );

    httpController.addStream( "file", input );
    httpController.addStream( "debug", SlaveVideo::cClass() );
    httpController.addStream( "debug", matrixVideo );
    httpController.addStream( "debug", motionDetector );

    app.run();
}

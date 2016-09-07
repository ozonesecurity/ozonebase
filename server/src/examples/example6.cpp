#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../processors/ozMatrixVideo.h"
#include "../protocols/ozHttpController.h"

#include "../libgen/libgenDebug.h"

//
// Run motion detection on a saved file, provide debug images in matrix mode over HTTP
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example6", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    AVInput input( "input", "/tmp/movie.mp4" );
    app.addThread( &input );

    MotionDetector motionDetector( "modect" );
    motionDetector.registerProvider( input );
    //EventRecorder eventRecorder( "/transfer/ozx" );
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

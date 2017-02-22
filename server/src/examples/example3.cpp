#include "../base/ozApp.h"
#include "../providers/ozAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../consumers/ozMovieFileOutput.h"
#include "../consumers/ozVideoRecorder.h"

#include "../libgen/libgenDebug.h"

//
// Load images from network video, timestamp image, write to MP4 file
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example3", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    //RemoteVideoInput input( "rtsp://test:test123@axis207.home/mpeg4/media.amp" );
    AVInput input( "input1", "rtsp://test:test@webcam.com/mpeg4/media.amp" );
    app.addThread( &input );

    //MotionDetector motionDetector( "detector" );
    //motionDetector.registerProvider( input );
    //app.addThread( &motionDetector );

    //VideoRecorder recorder( "recorder" , "/transfer", "mp4", videoParms, audioParms );
    Options outputOptions;
    outputOptions.set( "format", "mp4" );
    outputOptions.set( "duration", 60 );
    outputOptions.set( "width", 960 );
    outputOptions.set( "height", 540 );
    outputOptions.set( "frameRate", 25.0 );
    outputOptions.set( "videoBitRate", 2*1000*1000 );
    MovieFileOutput output( "output" , outputOptions );
    output.registerProvider( input );
    app.addThread( &output );

    app.run();
}

#include "../base/ozApp.h"
#include "../providers/ozNetworkAVInput.h"
#include "../processors/ozFaceDetector.h"
#include "../consumers/ozMovieFileOutput.h"
#include "../consumers/ozLocalFileOutput.h"

#include "../libgen/libgenDebug.h"

//
// Load images from network stream, run motion detection, write to MP4 files and trap and print notifications
// WARNING - This example can write lots of files, some large, to /tmp (or wherever configured)
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example10", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    NetworkAVInput input( "input", "/dev/video1" );
    //NetworkAVInput input( "input", "Shop Door Camera.mp4" );
    //NetworkAVInput input( "input", "mcem0_head.mpg" );
    app.addThread( &input );

    FaceDetector detector( "detector" );
    detector.registerProvider( input );
    app.addThread( &detector );

    // Writes out individual images
    //LocalFileOutput output( detector.cname(), "/transfer" );
    //output.registerProvider( detector );
    //app.addThread( &output );

    // Writes out movie
    VideoParms videoParms( 320, 240 );
    AudioParms audioParms;
    MovieFileOutput output2( detector.cname(), "/tmp", "mp4", 30, videoParms, audioParms );
    output2.registerProvider( detector );
    app.addThread( &output2 );

    app.run();
}

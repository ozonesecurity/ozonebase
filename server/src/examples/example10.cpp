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

    NetworkAVInput input( "input", "http://kxhcm10/nphMotionJpeg?Resolution=640x480&Quality=Standard", "mjpeg" );
    app.addThread( &input );

    FaceDetector detector( "detector" );
    detector.registerProvider( input );
    app.addThread( &detector );

    LocalFileOutput output( detector.cname(), "/transfer" );
    output.registerProvider( detector );
    app.addThread( &output );

    app.run();
}

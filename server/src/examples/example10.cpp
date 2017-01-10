#include "../base/ozApp.h"
#include "../providers/ozAVInput.h"
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

    AVInput input( "input", "/dev/video1" );
    //AVInput input( "input", "Shop Door Camera.mp4" );
    //AVInput input( "input", "mcem0_head.mpg" );
    app.addThread( &input );

    Options faceOptions;
    //faceOptions.set( "method", "hog" );
    //faceOptions.set( "dataFile", "shape_predictor_68_face_landmarks.dat" );
    faceOptions.set( "method", "cnn" );
    faceOptions.set( "dataFile", "mmod_human_face_detector.dat" );
    faceOptions.set( "markup", FaceDetector::OZ_FACE_MARKUP_ALL );
    FaceDetector detector( "detector", faceOptions );
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

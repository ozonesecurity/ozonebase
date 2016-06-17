#include "../base/ozApp.h"
#include "../providers/ozNetworkAVInput.h"
#include "../processors/ozImageTimestamper.h"
#include "../consumers/ozMovieFileOutput.h"

#include "../libgen/libgenDebug.h"

//
// Load images from network video, timestamp image, write to MP4 file
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example3", "", 5 );

    Info( "Starting" );

    ffmpegInit();

    Application app;

    //RemoteVideoInput input( "rtsp://test:test123@axis207.home/mpeg4/media.amp" );
    NetworkAVInput input( "input1", "rtsp://test:test@webcam.com/mpeg4/media.amp" );
    app.addThread( &input );

    ImageTimestamper timestamper( input.cname() );
    app.addThread( &timestamper );

    VideoParms videoParms( 640, 480 );
    AudioParms audioParms;
    MovieFileOutput output( input.cname(), "/tmp", "mp4", 300, videoParms, audioParms );
    output.registerProvider( timestamper );
    app.addThread( &output );

    app.run();
}

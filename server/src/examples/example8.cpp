#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozMemoryInputV1.h"
#include "../processors/ozFaceDetector.h"
#include "../consumers/ozVideoRecorder.h"
#include "../protocols/ozHttpController.h"
#include "../protocols/ozRtspController.h"

#include "../libgen/libgenDebug.h"

//
// Make images from ZM V1 shared memory available over the network
// You have to manually tune the shared mmemory sizes etc to match
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example8", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "rtsp", 8854, RtspController::PortRange( 28000, 28998 ) );
    listener.addController( &rtspController );

    HttpController httpController( "http", 8880 );
    listener.addController( &httpController );

    const int maxMonitors = 5;
    for ( int monitor = 5; monitor <= maxMonitors; monitor++ )
    {
        char idString[32] = "";

        sprintf( idString, "input%d", monitor );
        // Get the individual images from shared memory
		MemoryInputV1 *input = new MemoryInputV1( idString, "/dev/shm", monitor, 50, PIX_FMT_RGB24, 640, 480 );
        app.addThread( input );

        rtspController.addStream( "input", *input );
        httpController.addStream( "input", *input );

        sprintf( idString, "detect%d", monitor );
		FaceDetector *detector = new FaceDetector( idString ,"./shape_predictor_68_face_landmarks.dat");
		detector->registerProvider( *input );
		app.addThread( detector );

        rtspController.addStream( "detect", *detector );
        httpController.addStream( "detect", *detector );

		VideoParms videoParms( 640, 480 );
		AudioParms audioParms;

        sprintf( idString, "record%d", monitor );
		VideoRecorder *recorder = new VideoRecorder( idString , "/tmp", "mov", videoParms, audioParms );
		recorder->registerProvider( *detector );
		app.addThread( recorder );
    }

    app.run();
}

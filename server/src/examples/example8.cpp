#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozMemoryInputV1.h"
#include "../processors/ozRateLimiter.h"
#include "../processors/ozMotionDetector.h"
#include "../processors/ozFaceDetector.h"
//#include "../processors/ozRecognizer.h"
#include "../consumers/ozVideoRecorder.h"
#include "../consumers/ozMemoryTriggerV1.h"
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

    RtspController rtspController( "rtsp", 9292, RtspController::PortRange( 28000, 28998 ) );
    listener.addController( &rtspController );

    HttpController httpController( "http", 9280 );
    listener.addController( &httpController );

    const int maxMonitors = 2;
    for ( int monitor = 2; monitor <= maxMonitors; monitor++ )
    {
        char idString[32] = "";

        sprintf( idString, "input%d", monitor );
        // Get the individual images from shared memory
        MemoryInputV1 *input = new MemoryInputV1( idString, "/dev/shmX", monitor, 50, PIX_FMT_RGB24, 704, 576 );
        app.addThread( input );

        rtspController.addStream( "input", *input );
        httpController.addStream( "input", *input );

        sprintf( idString, "limit%d", monitor );
        RateLimiter *limiter = new RateLimiter( idString , 10 );
        //MotionDetector *detector = new MotionDetector( idString );
        limiter->registerProvider( *input );
        app.addThread( limiter );

        sprintf( idString, "detect%d", monitor );
        //FaceDetector *detector = new FaceDetector( idString ,"./shape_predictor_68_face_landmarks.dat");
        MotionDetector *detector = new MotionDetector( idString );
        //Recognizer *detector = new Recognizer( idString, Recognizer::OZ_RECOGNIZER_OPENALPR );
        detector->registerProvider( *limiter );
        app.addThread( detector );

        //rtspController.addStream( "detect", *detector );
        //httpController.addStream( "detect", *detector );

        //VideoParms videoParms( 640, 480 );
        //AudioParms audioParms;

        //sprintf( idString, "record%d", monitor );
        //VideoRecorder *recorder = new VideoRecorder( idString , "/transfer", "mp4", videoParms, audioParms );
        //recorder->registerProvider( *detector );
        //app.addThread( recorder );

        sprintf( idString, "trigger%d", monitor );
        MemoryTriggerV1 *trigger = new MemoryTriggerV1( idString , "/dev/shmX", monitor );
        trigger->registerProvider( *detector );
        app.addThread( trigger );
    }

    app.run();
}

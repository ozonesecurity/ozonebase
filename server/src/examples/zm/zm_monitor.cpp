#include <base/ozApp.h>
#include <base/ozListener.h>
#include <providers/ozMemoryInputV1.h>
#include <processors/ozRateLimiter.h>
#include <processors/ozShapeDetector.h>
#include <processors/ozFaceDetector.h>
#include <processors/ozAVFilter.h>
//#include processors/ozRecognizer.h>
#include <consumers/ozVideoRecorder.h>
#include <consumers/ozMemoryTriggerV1.h>
#include <libgen/libgenDebug.h>
#include <protocols/ozHttpController.h>
#include <processors/ozImageScale.h>
#include <stdio.h>
#include <iostream>
using namespace std;

//
// Make images from ZM V1 shared memory available over the network
// You have to manually tune the shared mmemory sizes etc to match
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "zm_monitor", "", 1 );

    Info( "Starting" );
    avInit();
    Application app;

    Listener listener;
    app.addThread( &listener );

    int monitor = 4; // monitor ID I want to intercept
    char idString[32] = "";
    sprintf( idString, "origmonitor%d", monitor );
	// you need to put in exact size here, also use 24bpp for now in ZM
    MemoryInputV1 *input = new MemoryInputV1( idString, "/dev/shm", monitor, 50, 1280, 960 );
    app.addThread( input );
    sprintf( idString, "limit%d", monitor );
    RateLimiter *limiter = new RateLimiter( idString , 10 );
    limiter->registerProvider( *input );
    app.addThread( limiter );

    sprintf( idString, "monitor%d", monitor );
	// Let's downsize image by half - dlib goes slow for large images
	VideoFilter *resizer = new VideoFilter("filter", "scale=iw/2:-1");
	resizer->registerProvider(*limiter);
	app.addThread(resizer);

    sprintf( idString, "detect%d", monitor );
    //ShapeDetector *detector  = new ShapeDetector( idString,"person.svm",ShapeDetector::OZ_SHAPE_MARKUP_OUTLINE  );
    FaceDetector *detector  = new FaceDetector( idString,"shape_predictor_68_face_landmarks.dat" );
    detector->registerProvider( *resizer );
    //detector->registerProvider( *input );
    app.addThread( detector );

	// set up feeds you can view on the side
    sprintf( idString, "trigger%d", monitor );
    MemoryTriggerV1 *trigger = new MemoryTriggerV1( idString , "/dev/shm", monitor );
    trigger->registerProvider( *detector );
    app.addThread( trigger );

    HttpController httpController( "http", 9292 );
    httpController.addStream( "live",*input );
    httpController.addStream( "detect", *detector );
    listener.addController( &httpController );

    cout << "Watching for shapes or faces in monitor:" << monitor << endl;
    app.run();
}

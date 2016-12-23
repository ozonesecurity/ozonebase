#include <base/ozApp.h>
#include <base/ozListener.h>
#include <providers/ozMemoryInputV1.h>
#include <providers/ozDummyInput.h>
#include <processors/ozRateLimiter.h>
#include <processors/ozShapeDetector.h>
#include <processors/ozFaceDetector.h>
#include <processors/ozMotionDetector.h>
#include <processors/ozAVFilter.h>
#include <processors/ozInputFallback.h>
//#include processors/ozRecognizer.h>
#include <consumers/ozVideoRecorder.h>
#include <consumers/ozMemoryTriggerV1.h>
#include <libgen/libgenDebug.h>
#include <protocols/ozHttpController.h>
#include <processors/ozImageScale.h>
#include <stdio.h>
#include <iostream>
using namespace std;

#define IMG_W 1280
#define IMG_H 960

bool alarmFramesOnly( FramePtr frame, const FeedConsumer * )
{
    const AlarmFrame *alarmFrame = dynamic_cast<const AlarmFrame *>(frame.get());
	if (alarmFrame->alarmed()) { cout << "MOTION DETECTED"; }
    return( alarmFrame->alarmed() );
}

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
    MemoryInputV1 *input = new MemoryInputV1( idString, "/dev/shm", monitor, 50,PIX_FMT_RGB24,  IMG_W, IMG_H );
    app.addThread( input );
    sprintf( idString, "limit%d", monitor );
    RateLimiter *limiter = new RateLimiter( idString , 10 );
    limiter->registerProvider( *input );
    app.addThread( limiter );

    sprintf( idString, "motion%d", monitor );
	Options mOptions;
	mOptions.add( "zone_default_alarmPercent_min", 0.1);
	MotionDetector *motion = new MotionDetector(idString,mOptions);
	motion->registerProvider(*limiter);
	app.addThread(motion);

    sprintf( idString, "monitor%d", monitor );
	// half
	VideoFilter *resizer = new VideoFilter("filter", "scale=iw/2:-1");
	// funky mirror
	// VideoFilter *resizer = new VideoFilter("filter", "crop=iw/2:ih:0:0,split[left][tmp];[tmp]hflip[right];[left][right] hstack");
	// waveform foo
	// VideoFilter *resizer = new VideoFilter("filter", "split[a][b];[a]waveform=e=3,split=3[c][d][e];[e]crop=in_w:20:0:235,lutyuv=v=180[low];[c]crop=in_w:16:0:0,lutyuv=y=val:v=180[high];[d]crop=in_w:220:0:16,lutyuv=v=110[mid] ; [b][high][mid][low]vstack=4");
	// histogram
	//VideoFilter *resizer = new VideoFilter("filter", "format=gbrp,split=4[a][b][c][d],[d]histogram=display_mode=0:level_height=244[dd],[a]waveform=m=1:d=0:r=0:c=7[aa],[b]waveform=m=0:d=0:r=0:c=7[bb],[c][aa]vstack[V],[bb][dd]vstack[V2],[V][V2]hstack");
	//edge detection
	//VideoFilter *resizer = new VideoFilter("filter", "edgedetect=low=0.1:high=0.4");
	//resizer->registerProvider(*limiter);
	//app.addThread(resizer);

    sprintf( idString, "detect%d", monitor );
    ShapeDetector *detector  = new ShapeDetector( idString,"person.svm",ShapeDetector::OZ_SHAPE_MARKUP_OUTLINE  );
    //FaceDetector *detector  = new FaceDetector( idString,"shape_predictor_68_face_landmarks.dat" );
    //detector->registerProvider( *resizer );
	detector->registerProvider( *motion, FeedLink(FEED_QUEUED, alarmFramesOnly) );
    app.addThread( detector );

	// set up feeds you can view on the side
    sprintf( idString, "trigger%d", monitor );
    MemoryTriggerV1 *trigger = new MemoryTriggerV1( idString , "/dev/shm", monitor );
    trigger->registerProvider( *detector );
    app.addThread( trigger );

	Options options;
    options.add( "width", IMG_W );
    options.add( "height", IMG_H );
    options.add( "pixelFormat", (PixelFormat)AV_PIX_FMT_RGB24 );
    DummyInput dummyInput( "dummy", options );
    app.addThread( &dummyInput );

	InputFallback fallbackInput( "fallback", 2.0 );
    fallbackInput.registerProvider( *detector );
    fallbackInput.registerProvider( dummyInput );
    app.addThread( &fallbackInput );

    HttpController httpController( "http", 9292 );
    httpController.addStream( "live",*input );
    httpController.addStream( "detect", *detector );
    httpController.addStream( "detect", *motion );
    httpController.addStream( "fallback", dummyInput );
    httpController.addStream( "fallback", fallbackInput );
    listener.addController( &httpController );

    cout << "Watching for shapes or faces in monitor:" << monitor << endl;
    app.run();
}

// ----------------------------------------
// NOT AN EXAMPLE - latest experiments
// may not work. Rather, may work if you are lucky
//------------------------------------------
#include "../base/zmApp.h"
#include "../base/zmListener.h"
#include "../providers/zmNetworkAVInput.h"
#include "../processors/zmMotionDetector.h"
#include "../processors/zmQuadVideo.h"
#include "../protocols/zmHttpController.h"
#include "../consumers/zmEventRecorder.h"
#include "../consumers/zmMovieFileOutput.h"

#include "../libgen/libgenDebug.h"

#include <iostream>

//
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "starter-example", "", 5 );

	std::cout << " ---------------------- Starter Example ------------------\n"
			" do a export PRINT_DBG=0/1 to turn off/on logs\n"
			" once running, load up starter-example.html in your browser\n"
			" You should see a traffic cam window as well as motion detection frames in another\n"
			" in real-time\n";

    Info( "Starting" );

    ffmpegInit();

    Application app;

   	// Two RTSP sources 
    NetworkAVInput cam1( "cam1", "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
    NetworkAVInput cam2("cam2","rtsp://170.93.143.139:1935/rtplive/e0ffa81e00a200ab0050fa36c4235c0a");
    app.addThread( &cam1 );
    app.addThread( &cam2 );

	// motion detect for cam1
	MotionDetector motionDetector1( "modectcam1" );
  	motionDetector1.registerProvider( cam1 );
   	app.addThread( &motionDetector1 );

	// motion detect for cam1
	MotionDetector motionDetector2( "modectcam2" );
  	motionDetector2.registerProvider( cam2 );
   	app.addThread( &motionDetector2 );

	// event recording to images
	EventRecorder *recorder = new EventRecorder( "motionimg-cam2", "/tmp" );
	recorder->registerProvider(motionDetector2);
	app.addThread(recorder);


	// event recording to a video
	VideoParms vid(640,480);
	AudioParms aud;
	MovieFileOutput mout ("motionvid-cam2", "/tmp", "mp4", 300, vid, aud);
	mout.registerProvider(motionDetector2);
	app.addThread(&mout);



	// Let's make a mux/stitched handler for cam1 and cam2 and its debugs
	QuadVideo quadVideo( "quadcammux", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
   	quadVideo.registerProvider( cam1 );
   	quadVideo.registerProvider( *motionDetector1.deltaImageSlave() );
   	quadVideo.registerProvider( cam2 );
   	quadVideo.registerProvider( *motionDetector2.deltaImageSlave() );
   	app.addThread( &quadVideo );


	Listener listener;
    app.addThread( &listener );

    HttpController httpController( "watch", 9292 );
    httpController.addStream("watchcam1",cam1);
    httpController.addStream("watchcam2",cam2);

	httpController.addStream( "file", cam1 );
   	httpController.addStream( "debug", SlaveVideo::cClass() );
   	httpController.addStream( "debug", quadVideo );
   	httpController.addStream( "debug", motionDetector1 );
   	httpController.addStream( "debug", motionDetector2 );
	
    listener.addController( &httpController );


    app.run();
}

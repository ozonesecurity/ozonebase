#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozNetworkAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../processors/ozMatrixVideo.h"
#include "../protocols/ozHttpController.h"

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

    avInit();

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

	// Let's make a mux/stitched handler for cam1 and cam2 and its debugs
	MatrixVideo matrixVideo( "matrixcammux", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
   	matrixVideo.registerProvider( cam1 );
   	matrixVideo.registerProvider( *motionDetector1.deltaImageSlave() );
   	matrixVideo.registerProvider( cam2 );
   	matrixVideo.registerProvider( *motionDetector2.deltaImageSlave() );
   	app.addThread( &matrixVideo );


	Listener listener;
    app.addThread( &listener );

    HttpController httpController( "watch", 9292 );
    httpController.addStream("watchcam1",cam1);
    httpController.addStream("watchcam2",cam2);

	httpController.addStream( "file", cam1 );
   	httpController.addStream( "debug", SlaveVideo::cClass() );
   	httpController.addStream( "debug", matrixVideo );
   	httpController.addStream( "debug", motionDetector1 );
   	httpController.addStream( "debug", motionDetector2 );
	
    listener.addController( &httpController );


    app.run();
}

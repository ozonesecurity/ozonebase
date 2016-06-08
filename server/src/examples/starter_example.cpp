#include "../base/zmApp.h"
#include "../base/zmListener.h"
#include "../providers/zmNetworkAVInput.h"
#include "../processors/zmMotionDetector.h"
#include "../processors/zmQuadVideo.h"
#include "../protocols/zmHttpController.h"

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

    NetworkAVInput input( "input", "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
    app.addThread( &input );

	MotionDetector motionDetector( "modect" );
  	motionDetector.registerProvider( input );
   	app.addThread( &motionDetector );

	QuadVideo quadVideo( "quad", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
   	quadVideo.registerProvider( *motionDetector.refImageSlave() );
   	quadVideo.registerProvider( *motionDetector.compImageSlave() );
   	quadVideo.registerProvider( *motionDetector.deltaImageSlave() );
   	quadVideo.registerProvider( *motionDetector.varImageSlave() );
   	app.addThread( &quadVideo );

    Listener listener;
    app.addThread( &listener );

    HttpController httpController( "watch", 9292 );
    httpController.addStream("watch",input);

	httpController.addStream( "file", input );
   	httpController.addStream( "debug", SlaveVideo::cClass() );
   	httpController.addStream( "debug", quadVideo );
   	httpController.addStream( "debug", motionDetector );
	
    listener.addController( &httpController );


    app.run();
}

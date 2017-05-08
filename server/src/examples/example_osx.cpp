#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../processors/ozImageConvert.h"
#include "../processors/ozMatrixVideo.h"
#include "../protocols/ozHttpController.h"

#include "../libgen/libgenDebug.h"
#include <unistd.h>


class nvrCamera
{
public:
    AVInput *cam;
    Detector *motion; 
    ImageConvert *convert;

};





int main( int argc, const char *argv[] )
{

    Listener *listener;
    HttpController* httpController;
    Application app;



    nvrCamera nvrcam;

    debugInitialise( "example_osx", "", 5 );

    Info( "Starting" );

    avInit();

    
    //listener->addController( httpController );

   
    Options avOptions;
    avOptions.add("format","avfoundation");
    avOptions.add("framerate","30");
    avOptions.add("videosize","640x480");

   
    nvrcam.cam = new AVInput ( "cam0", "0",avOptions );
    nvrcam.convert = new ImageConvert ( "convert",PIX_FMT_RGB24, 640,480 );
    nvrcam.motion = new MotionDetector( "modect-cam0" );
    nvrcam.convert->registerProvider(*(nvrcam.cam) );
    nvrcam.motion->registerProvider(*(nvrcam.convert) );
     //nvrcam.motion->registerProvider(*(nvrcam.cam) );

    nvrcam.convert->start();
    nvrcam.motion->start();
    nvrcam.cam->start();

    listener = new Listener;
    httpController = new HttpController( "watch", 9292 );
    httpController->addStream("live",*(nvrcam.cam));
    httpController->addStream("debug",*(nvrcam.motion));
    listener->addController(httpController);

     app.addThread( listener );
    // app.run();
    // 
    listener->start();


    while (1) { usleep(2000);};
    //app.addThread( &input );

   // app.run();
}

#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../processors/ozMatrixVideo.h"
#include "../protocols/ozHttpController.h"

#include "../libgen/libgenDebug.h"
#include <unistd.h>


class nvrCamera
{
public:
    AVInput *cam;
    Detector *motion; 
};



int main( int argc, const char *argv[] )
{
    nvrCamera nvrcam;
    debugInitialise( "example_osx", "", 5 );

    Info( "Starting" );

    avInit();

    Application app;
    Options avOptions;
    avOptions.add("format","avfoundation");
    avOptions.add("framerate","30");

   
    nvrcam.cam = new AVInput ( "input", "0",avOptions );


    nvrcam.motion = new MotionDetector( "modect-input" );
    nvrcam.motion->registerProvider(*(nvrcam.cam) );

    nvrcam.motion->start();
    nvrcam.cam->start();


    while (1) { usleep(2000);};
    //app.addThread( &input );

   // app.run();
}

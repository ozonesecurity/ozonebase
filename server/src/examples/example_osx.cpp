#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../processors/ozMatrixVideo.h"
#include "../protocols/ozHttpController.h"

#include "../libgen/libgenDebug.h"

int main( int argc, const char *argv[] )
{
    debugInitialise( "example1", "", 5 );

    Info( "Starting" );

    avInit();

    Application app;
    Options avOptions;
    avOptions.add("format","avfoundation");
    avOptions.add("framerate","30");

    AVInput input( "input", "0",avOptions );
    app.addThread( &input );

    app.run();
}

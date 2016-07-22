#include "../base/ozApp.h"
#include "../providers/ozMemoryInput.h"
#include "../processors/ozMotionDetector.h"
#include "../consumers/ozEventRecorder.h"
#include "../consumers/ozLocalFileOutput.h"

#include "../libgen/libgenDebug.h"

//
// Fetch frames from shared mempory and run motion detection
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example2", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    const int maxMonitors = 1;
    for ( int monitor = 1; monitor <= maxMonitors; monitor++ )
    {
        char idString[32] = "";

        // Get the individual images from shared memory
        sprintf( idString, "imageInput%d", monitor );
        MemoryInput *imageInput = new MemoryInput( idString, "/dev/shm", monitor );
        app.addThread( imageInput );

        // Run motion detection on the images
        sprintf( idString, "detector%d", monitor );
        MotionDetector *detector = new MotionDetector( idString );
        detector->registerProvider( *imageInput );
        app.addThread( detector );

        // Frame based event recorder, only writes images when alarmed
        sprintf( idString, "recorder%d", monitor );
        EventRecorder *recorder = new EventRecorder( idString, "/tmp" );
        recorder->registerProvider( *detector );
        app.addThread( recorder );

        // File output
        sprintf( idString, "output%d", monitor );
        LocalFileOutput *output = new LocalFileOutput( idString, "/tmp" );
        output->registerProvider( *imageInput );
        app.addThread( output );
    }

    app.run();
}

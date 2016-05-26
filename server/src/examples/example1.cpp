#include "../base/zmApp.h"
#include "../providers/zmLocalVideoInput.h"
#include "../consumers/zmMemoryOutput.h"

#include "../libgen/libgenDebug.h"

//
// Capture from local V4L2 device and write to ZM V2 compatible shared memory
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example1", "", 5 );

    Info( "Starting" );

    ffmpegInit();

    Application app;

    LocalVideoInput input( "input1", "/dev/video0" );
    app.addThread( &input );

    MemoryOutput memoryOutput( input.cname(), "/dev/shm", 10 );
    memoryOutput.registerProvider( input );
    app.addThread( &memoryOutput );

    app.run();
}

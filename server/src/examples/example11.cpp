#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozDummyInput.h"
#include "../providers/ozMemoryInputV1.h"
#include "../processors/ozInputFallback.h"
#include "../protocols/ozHttpController.h"

#include "../libgen/libgenDebug.h"

//
// Take a feed from two sources. Use the first unless it fails then fallback to 
// the second. Revert back if the original feed returns
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "example11", "", 0 );

    Info( "Starting" );

    avInit();

    Application app;

    Listener listener;
    app.addThread( &listener );

    HttpController httpController( "p9280", 9280 );
    listener.addController( &httpController );

    MemoryInputV1 memoryInput( "memory", "/dev/shmX", 1, 50, PIX_FMT_RGB24, 704, 576 );
    app.addThread( &memoryInput );

    Options options;
    options.add( "width", 704 );
    options.add( "height", 576 );
    options.add( "pixelFormat", (PixelFormat)AV_PIX_FMT_RGB24 );
    DummyInput dummyInput( "dummy", options );
    app.addThread( &dummyInput );

    InputFallback fallbackInput( "fallback", 5.0 );
    fallbackInput.registerProvider( memoryInput );
    fallbackInput.registerProvider( dummyInput );
    app.addThread( &fallbackInput );

    httpController.addStream( "input", memoryInput );
    httpController.addStream( "input", dummyInput );
    httpController.addStream( "input", fallbackInput );

    app.run();
}

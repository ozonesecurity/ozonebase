#include "../base/zm.h"
#include "../base/zmApp.h"
#include "../base/zmFeedProvider.h"
#include "../base/zmFeedConsumer.h"
#include "../base/zmListener.h"
#include "../base/zmSignal.h"
#include "../providers/zmLocalVideoInput.h"
#include "../providers/zmLocalFileInput.h"
#include "../providers/zmRemoteVideoInput.h"
#include "../providers/zmNetworkAVInput.h"
#include "../providers/zmMemoryInput.h"
#include "../providers/zmMemoryInputV1.h"
#include "../providers/zmVideo4LinuxInput.h"
#include "../providers/zmRawH264Input.h"
#include "../processors/zmImageTimestamper.h"
#include "../processors/zmSignalChecker.h"
#include "../processors/zmMotionDetector.h"
#include "../processors/zmVideoFilter.h"
#include "../processors/zmRateLimiter.h"
#include "../processors/zmQuadVideo.h"
#include "../processors/zmFilterSwapUV.h"
#include "../encoders/zmJpegEncoder.h"
#include "../consumers/zmLocalFileOutput.h"
#include "../consumers/zmLocalFileDump.h"
#include "../consumers/zmMemoryOutput.h"
#include "../consumers/zmEventRecorder.h"
#include "../consumers/zmMovieFileOutput.h"
//#include "../consumers/zmMp4FileOutput.h"
#include "../protocols/zmHttpController.h"
#include "../protocols/zmRtspController.h"
#include "../protocols/zmRtmpController.h"

#include "../libgen/libgenDebug.h"

//
// Capture from local V4L2 device and write to ZM V2 compatible shared memory
//
void example1()
{
    Application app;

    LocalVideoInput input( "input1", "/dev/video0" );
    app.addThread( &input );

    MemoryOutput memoryOutput( input.cname(), "/dev/shm", 10 );
    memoryOutput.registerProvider( input );
    app.addThread( &memoryOutput );

    app.run();
}

//
// Fetch frames from shared mempory and run motion detection
//
void example2()
{
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

//
// Load images from network video, timestamp image, write to MP4 file
//
void example3()
{
    Application app;

    //RemoteVideoInput input( "rtsp://test:test123@axis207.home/mpeg4/media.amp" );
    NetworkAVInput input( "input1", "rtsp://test:test@webcam.com/mpeg4/media.amp" );
    app.addThread( &input );

    ImageTimestamper timestamper( input.cname() );
    app.addThread( &timestamper );

    VideoParms videoParms( 640, 480 );
    AudioParms audioParms;
    MovieFileOutput output( input.cname(), "/tmp", "mp4", 300, videoParms, audioParms );
    output.registerProvider( timestamper );
    app.addThread( &output );

    app.run();
}

//
// Read video file and stream over Http and RTSP
//
void example4()
{
    Application app;

    NetworkAVInput input( "input", "/tmp/movie.mp4" );
    app.addThread( &input );

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    HttpController httpController( "p8080", 8080 );

    rtspController.addStream( "raw", input );
    httpController.addStream( "raw", input );

    listener.addController( &rtspController );
    listener.addController( &httpController );
}

//
// Load one file, one V4L and two network streams, combine to quad video available over RTSP
//
void example5()
{
    Application app;

    LocalVideoInput input1( "input1", "/dev/video0" );
    app.addThread( &input1 );
    NetworkAVInput input2( "input1", "rtsp://test:test@webcam1/mpeg4/media.amp" );
    app.addThread( &input2 );
    NetworkAVInput input3( "input1", "rtsp://test:test@webcam2/mpeg4/media.amp" );
    app.addThread( &input3 );
    NetworkAVInput input4( "input", "/tmp/movie.mp4" );
    app.addThread( &input4 );

    QuadVideo quadVideo( "quad", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    quadVideo.registerProvider( input1 );
    quadVideo.registerProvider( input2 );
    quadVideo.registerProvider( input3 );
    quadVideo.registerProvider( input4 );
    app.addThread( &quadVideo );

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    listener.addController( &rtspController );

    rtspController.addStream( "input1", input1 );
    rtspController.addStream( "input2", input2 );
    rtspController.addStream( "input3", input3 );
    rtspController.addStream( "input4", input4 );
    rtspController.addStream( "quad", quadVideo );

    app.run();
}

//
// Run motion detection on a saved file, provide debug images in quad mode over HTTP
//
void example6()
{
    Application app;

    NetworkAVInput input( "input", "/tmp/movie.mp4" );
    app.addThread( &input );

    MotionDetector motionDetector( "modect" );
    motionDetector.registerProvider( input );
    //EventRecorder eventRecorder( "/transfer/zmx" );
    app.addThread( &motionDetector );

    QuadVideo quadVideo( "quad", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    quadVideo.registerProvider( *motionDetector.refImageSlave() );
    quadVideo.registerProvider( *motionDetector.compImageSlave() );
    quadVideo.registerProvider( *motionDetector.deltaImageSlave() );
    quadVideo.registerProvider( *motionDetector.varImageSlave() );
    app.addThread( &quadVideo );

    Listener listener;
    app.addThread( &listener );

    HttpController httpController( "p8080", 8080 );
    listener.addController( &httpController );

    httpController.addStream( "file", input );
    httpController.addStream( "debug", SlaveVideo::cClass() );
    httpController.addStream( "debug", quadVideo );
    httpController.addStream( "debug", motionDetector );

    app.run();
}

//
// Relay H.264 packets directly from source
//
void example7()
{
    Application app;

    RawH264Input input( "input", "rtsp://Admin:123456@10.10.10.10:7070" );
    app.addThread( &input );

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    listener.addController( &rtspController );

    rtspController.addStream( "raw", input );

    app.run();
}

//
// Make images from ZM V1 shared memory available over the network
// You have to manually tune the shared mmemory sizes etc to match
//
void example8()
{
    Application app;

    Listener listener;
    app.addThread( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 28000, 28998 ) );
    listener.addController( &rtspController );

    HttpController httpController( "p8080", 8080 );
    listener.addController( &httpController );

    const int maxMonitors = 1;
    for ( int monitor = 1; monitor <= maxMonitors; monitor++ )
    {
        char idString[32] = "";

        // Get the individual images from shared memory
        sprintf( idString, "input%d", monitor );
        MemoryInputV1 *input = new MemoryInputV1( idString, "/dev/shm", monitor, 75, PIX_FMT_RGB24, 1920, 1080 );
        app.addThread( input );

        rtspController.addStream( "raw", *input );
        httpController.addStream( "raw", *input );
    }

    app.run();
}

int main( int argc, const char *argv[] )
{
    debugInitialise( "zmx", "", 5 );

    Info( "Starting" );

    ffmpegInit();

    example1();
    example2();
    example3();
    example4();
    example5();
    example6();
    example7();
    example8();
}

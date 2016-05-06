#include "zm.h"
#include "zmFeedProvider.h"
#include "zmFeedConsumer.h"
#include "providers/zmLocalVideoInput.h"
#include "providers/zmLocalFileInput.h"
#include "providers/zmRemoteVideoInput.h"
#include "providers/zmNetworkAVInput.h"
#include "providers/zmMemoryInput.h"
#include "providers/zmVideo4LinuxInput.h"
#include "providers/zmYuanInput.h"
#include "processors/zmImageTimestamper.h"
#include "processors/zmSignalChecker.h"
#include "processors/zmMotionDetector.h"
#include "processors/zmVideoFilter.h"
#include "processors/zmRateLimiter.h"
#include "processors/zmQuadVideo.h"
#include "encoders/zmJpegEncoder.h"
#include "consumers/zmLocalFileOutput.h"
#include "consumers/zmLocalFileDump.h"
#include "consumers/zmMemoryOutput.h"
#include "consumers/zmEventRecorder.h"
#include "consumers/zmMovieFileOutput.h"
#include "zmListener.h"
#include "protocols/zmHttpController.h"
#include "protocols/zmRtspController.h"
#include "protocols/zmRtmpController.h"
#include "zmSignal.h"

#include "libgen/libgenDebug.h"

VideoProvider *gInput = 0;

int main( int argc, const char *argv[] )
{
    debugInitialise( "zmx", "", 5 );

    Info( "Starting" );

    srand( time( NULL ) );

    zmSetDefaultTermHandler();

    //setDefaultDieHandler();

    ffmpegInit();

    typedef std::deque<Thread *> ThreadList;

    ThreadList threads;

#if 0
    //LocalVideoInput input( "/dev/video2" );
    //RemoteVideoInput input( "http://axis210.home/axis-cgi/mjpg/video.cgi?resolution=320x240", "mjpeg" );
    RemoteVideoInput input( "rtsp://test:test123@axis207.home/mpeg4/media.amp" );
    MemoryOutput memoryOutput( "/dev/shm", 100 );

    memoryOutput.registerProvider( input );

    input.start();
    memoryOutput.start();

    // Do stuff
    while( true )
        sleep( 1 );
  
    memoryOutput.join();
    input.join();
#endif

#if 0
    MemoryInput memoryInput( "/dev/shm", 100 );
    LocalFileOutput fileOutput( "/transfer" );

    fileOutput.registerProvider( memoryInput );

    memoryInput.start();
    fileOutput.start();

    // Do stuff
    while( true )
        sleep( 1 );
  
    fileOutput.join();
    memoryInput.join();
#endif

#if 0
    //LocalVideoInput input( "/dev/video2" );
    //RemoteVideoInput input( "http://axis210.home/axis-cgi/mjpg/video.cgi?resolution=320x240", "mjpeg" );
    RemoteVideoInput input( "rtsp://test:test123@axis207.home/mpeg4/media.amp" );
    //RemoteVideoInput input( "http://test:test123@axis207.home/axis-cgi/mjpg/video.cgi?resolution=320x240&req_fps=5", "mjpeg" );

    SignalChecker signalChecker;
    MotionDetector motionDetector;
    //LocalFileOutput fileOutput( stringtf( "/transfer/zmx-%d", getpid() ) );
    LocalFileOutput fileOutput( "/transfer/zmx" );
    EventRecorder eventRecorder( "/transfer/zmx" );
    MemoryOutput memoryOutput( "/dev/shm", 100 );

    signalChecker.registerProvider( input );
    fileOutput.registerProvider( signalChecker, SignalChecker::signalInvalid );
    memoryOutput.registerProvider( signalChecker, SignalChecker::signalValid );
    motionDetector.registerProvider( signalChecker, SignalChecker::signalValid );
    eventRecorder.registerProvider( motionDetector );
    LocalFileOutput fileOutput( "/transfer/zmx" );
    fileOutput.registerProvider( motionDetector, MotionDetector::inAlarm );
    fileOutput.registerProvider( motionDetector, MotionDetector::inAlarm );
    //fileOutput.registerProvider( input );

    srand( time( NULL ) );

    input.start();
    signalChecker.start();
    motionDetector.start();
    fileOutput.start();
    eventRecorder.start();
    memoryOutput.start();

    // Do stuff
    while( !gZmTerminate )
        sleep( 1 );
  
    memoryOutput.stop();
    eventRecorder.stop();
    fileOutput.stop();
    motionDetector.stop();
    signalChecker.stop();
    input.stop();

    memoryOutput.join();
    eventRecorder.join();
    fileOutput.join();
    motionDetector.join();
    signalChecker.join();
    input.join();
#endif

#if 0
    //RemoteVideoInput input( "rtsp://test:test123@axis207.home/mpeg4/media.amp" );
    //RemoteVideoInput input( "rtsp://axisp3301.home/mpeg4/media.amp" );
    //RemoteVideoInput input( "http://axisp3301.home/axis-cgi/mjpg/video.cgi?resolution=320x240&req_fps=5", "mjpeg" );
    //NetworkAVInput input( "rtsp://test:test123@axis207.home/mpeg4/media.amp" );

    LocalVideoInput video0( "video0", "/dev/video0" );
    //threads.push_back( &video0 );
    Video4LinuxInput video2( "video2", "/dev/video2", V4L2_STD_PAL, V4L2_PIX_FMT_YUV420, 640, 480 );
    //threads.push_back( &video2 );

    RemoteVideoInput axis207( "axis207", "http://test:test123@axis207.home/axis-cgi/mjpg/video.cgi?resolution=320x240&req_fps=5", "mjpeg" );
    threads.push_back( &axis207 );
    RemoteVideoInput axis210( "axis210", "http://axis210.home/axis-cgi/mjpg/video.cgi?resolution=320x240&req_fps=5", "mjpeg" );
    threads.push_back( &axis210 );
    NetworkAVInput p3301( "p3301", "rtsp://axisp3301.home/axis-media/media.amp" );
    //threads.push_back( &p3301 );
    NetworkAVInput axis206m( "axis206m", "http://axis206m.home/axis-cgi/mjpg/video.cgi?resolution=352x288&req_fps=10", "mjpeg" );
    threads.push_back( &axis206m );
    NetworkAVInput acti4200( "acti4200", "rtsp://admin:123456@acti4200.home:7070" );
    //threads.push_back( &acti4200 );
    NetworkAVInput acti7300( "acti7300", "rtsp://Admin:123456@acti7300.home:7070" );
    //threads.push_back( &acti7300 );

    ImageTimestamper imageTimestamper1( axis207.cname() );
    imageTimestamper1.registerProvider( axis207 );
    threads.push_back( &imageTimestamper1 );
    ImageTimestamper imageTimestamper2( p3301, p3301.videoFramesOnly );
    threads.push_back( &imageTimestamper2 );
    ImageTimestamper imageTimestamper3( axis210 );
    threads.push_back( &imageTimestamper3 );

    QuadVideo quadVideo( "quad", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    quadVideo.registerProvider( axis207 );
    //quadVideo.registerProvider( video0 );
    quadVideo.registerProvider( axis210 );
    quadVideo.registerProvider( axis206m );
    //threads.push_back( &quadVideo );

    //PolledRateLimiter rateLimiter( 1.0, imageTimestamper2 );
    VideoParms videoParms;
    AudioParms audioParms;
    MovieFileOutput movieOutput( "movie", "/tmp", "mp4", 300, videoParms, audioParms );
    movieOutput.registerProvider( axis206m );
    threads.push_back( &movieOutput );

    //QueuedVideoFilter videoFilter( p3301 );

    //Listener listener( "127.0.0.1" );
    Listener listener;
    threads.push_back( &listener );

    HttpController httpController( "p8080", 8080 );
    //RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    RtmpController rtmpController( "p1935", 1935 );

    httpController.addStream( "live", ImageTimestamper::cClass() );
    httpController.addStream( "raw", NetworkAVInput::cClass() );
    //rtspController.addStream( "live", imageTimestamper1 );
    //rtspController.addStream( "live", imageTimestamper2 );
    rtmpController.addStream( "live", ImageTimestamper::cClass() );
    rtmpController.addStream( "live", imageTimestamper2 );
    rtmpController.addStream( "live", imageTimestamper3 );
    rtmpController.addStream( "raw", axis207 );
    rtmpController.addStream( "raw", p3301 );
    rtmpController.addStream( "raw", axis210 );
    rtmpController.addStream( "raw", video0 );
    rtmpController.addStream( "raw", video2 );
    rtmpController.addStream( "raw", acti4200 );
    rtmpController.addStream( "raw", acti7300 );
    rtmpController.addStream( "raw", axis206m );
    rtmpController.addStream( "quad", quadVideo );
    httpController.addStream( "quad", quadVideo );
    //httpController.addStream( "slow", rateLimiter );
    //rtmpController.addStream( "raw", videoFilter );
    //rtspController.addStream( "raw", p3301 );

    listener.addController( &httpController );
    //listener.addController( &rtspController );
    listener.addController( &rtmpController );
#endif

#if 0
    //LocalVideoInput video1( "video1", "/dev/video1" );
    //threads.push_back( &video1 );
    //Video4LinuxInput video2( "video2", "/dev/video2", V4L2_STD_PAL, V4L2_PIX_FMT_YUV420, 640, 480 );
    //threads.push_back( &video2 );

    //RemoteVideoInput axis207( "axis207", "http://test:test123@axis207.home/axis-cgi/mjpg/video.cgi?resolution=320x240&req_fps=5", "mjpeg" );
    //threads.push_back( &axis207 );
    //RemoteVideoInput axis210( "axis210", "http://axis210.home/axis-cgi/mjpg/video.cgi?resolution=320x240&req_fps=5", "mjpeg" );
    //threads.push_back( &axis210 );
    //NetworkAVInput axis210( "axis210", "rtsp://axis210.home/mpeg/media.amp" );
    //threads.push_back( &axis210 );
    //threads.push_back( &p3301 );

#if 1
    //NetworkAVInput motionInput( "motionInput", "rtsp://axisp3301.home/axis-media/media.amp" );
    //NetworkAVInput motionInput( "axis206m", "http://axis206m.home/axis-cgi/mjpg/video.cgi?resolution=352x288&req_fps=10", "mjpeg" );
    LocalVideoInput motionInput( "motionInput", "/dev/video0" );
    threads.push_back( &motionInput );
#else
    NetworkAVInput file( "file", "/transfer/video0-1326362926.mp4" );
    //LocalFileInput file( "file", "/var/www/html/zm/events/PTZ/12/01/12/08/21/12/???-capture.jpg", 10 );
    threads.push_back( &file );
    RateLimiter motionInput( 25, false, file );
    threads.push_back( &motionInput );
#endif

    MotionDetector motionDetector( "modect" );
    motionDetector.registerProvider( motionInput );
    //EventRecorder eventRecorder( "/transfer/zmx" );
    threads.push_back( &motionDetector );

    //LocalFileOutput fileOutput( "zmxout", "/transfer/zmx" );
    //fileOutput.registerProvider( motionDetector, MotionDetector::inAlarm );
    //threads.push_back( &fileOutput );

    QuadVideo quadVideo( "quad", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    quadVideo.registerProvider( *motionDetector.refImageSlave() );
    quadVideo.registerProvider( *motionDetector.compImageSlave() );
    quadVideo.registerProvider( *motionDetector.deltaImageSlave() );
    quadVideo.registerProvider( *motionDetector.varImageSlave() );
    threads.push_back( &quadVideo );

    Listener listener;
    threads.push_back( &listener );

    HttpController httpController( "p8080", 8080 );
    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    RtmpController rtmpController( "p1935", 1935 );

    httpController.addStream( "raw", motionInput );
    httpController.addStream( "debug", SlaveVideo::cClass() );
    httpController.addStream( "debug", quadVideo );
    httpController.addStream( "debug", motionDetector );
    rtspController.addStream( "raw", motionInput );
    rtspController.addStream( "debug", SlaveVideo::cClass() );
    rtspController.addStream( "debug", quadVideo );
    rtspController.addStream( "debug", motionDetector );
    rtmpController.addStream( "raw", motionInput );
    rtmpController.addStream( "debug", SlaveVideo::cClass() );
    rtmpController.addStream( "debug", quadVideo );
    rtmpController.addStream( "debug", motionDetector );

    listener.addController( &httpController );
    listener.addController( &rtspController );
    listener.addController( &rtmpController );
#endif

#if 0
    // Read from analogue video and write to video files, 5 mins each
    LocalVideoInput video0( "video0", "/dev/video0" );
    threads.push_back( &video0 );

    VideoParms videoParms( 352, 288 );
    AudioParms audioParms;
    MovieFileOutput movieOutput( "video0", "/transfer", "mp4", 300, videoParms, audioParms );
    movieOutput.registerProvider( video0 );
    threads.push_back( &movieOutput );
#endif

#if 1
    // RTSP/H.264 Streaming

    //NetworkAVInput input( "input", "rtsp://axisp3301.home/axis-media/media.amp" );
    //NetworkAVInput input( "input", "http://axis206m.home/axis-cgi/mjpg/video.cgi?resolution=352x288&req_fps=10", "mjpeg" );
    //LocalVideoInput input( "input", "/dev/video0" );
    //Video4LinuxInput input( "input", "/dev/video0", V4L2_STD_PAL, V4L2_PIX_FMT_YUV420, 720, 576 );
    //YuanInput input( "input", "/dev/video0", V4L2_STD_PAL, V4L2_PIX_FMT_YVU420, 1920, 1080, false );
    YuanInput input( "input", "/dev/video1", V4L2_STD_PAL, V4L2_PIX_FMT_UYVY, 1920, 1080, true );
    //Video4LinuxInput input( "input", "/dev/video0", V4L2_STD_PAL, V4L2_PIX_FMT_YUYV, 1024, 768 );
    //Video4LinuxInput input( "input", "/dev/video0", V4L2_STD_PAL, V4L2_PIX_FMT_RGB24, 1024, 768 );
    //NetworkAVInput input( "input", "/transfer/video0-1326362926.mp4" );
    threads.push_back( &input );

    LocalFileDump fileDump( "dump", "/transfer" );
    fileDump.registerProvider( input );
    threads.push_back( &fileDump );

    Listener listener;
    threads.push_back( &listener );

    RtspController rtspController( "p8554", 8554, RtspController::PortRange( 58000, 58998 ) );
    HttpController httpController( "p8080", 8080 );

    rtspController.addStream( "raw", input );
    httpController.addStream( "raw", input );

    listener.addController( &rtspController );
    listener.addController( &httpController );
#endif
    for ( ThreadList::iterator iter = threads.begin(); iter != threads.end(); iter++ )
        (*iter)->start();

    // Do stuff
    while( !gZmTerminate )
        sleep( 1 );

    for ( ThreadList::iterator iter = threads.begin(); iter != threads.end(); iter++ )
        (*iter)->stop();

    for ( ThreadList::iterator iter = threads.begin(); iter != threads.end(); iter++ )
        (*iter)->join();
}

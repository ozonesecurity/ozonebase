#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozNetworkAVInput.h"
#include "../processors/ozMotionDetector.h"
#include "../processors/ozFaceDetector.h"
#include "../processors/ozShapeDetector.h"
#include "../processors/ozRateLimiter.h"
#include "../processors/ozImageScale.h"
#include "../processors/ozAVFilter.h"
#include "../processors/ozMatrixVideo.h"
#include "../protocols/ozHttpController.h"

#include "../libgen/libgenDebug.h"

#include <iostream>

//
//
int main( int argc, const char *argv[] )
{
    debugInitialise( "starter-example", "", 0 );

    std::cout << " ---------------------- Starter Example ------------------\n"
            " do a export DBG_PRINT=0/1 to turn off/on logs\n"
            " once running, load up starter-example.html in your browser\n"
            " You should see a traffic cam window as well as motion detection frames in another\n"
            " in real-time\n";

    Info( "Starting" );

    avInit();

    Application app;

    // Two RTSP sources 
    NetworkAVInput traffic( "traffic", "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
    NetworkAVInput people( "people", "face-input.mp4", "", true );
    //app.addThread( &traffic );
    app.addThread( &people );

    // motion detect for traffic
    MotionDetector trafficDetector( "traffic" );
    trafficDetector.registerProvider( traffic );
    //app.addThread( &trafficDetector );

    // rate limiter for people detector
    RateLimiter peopleLimiter( "people-limiter", 5 );
    peopleLimiter.registerProvider( people, FeedLink( FEED_QUEUED, AudioVideoProvider::videoFramesOnly ) );
    app.addThread( &peopleLimiter );

    // Scale video up
    //ImageScale peopleScalePre( "people-scale-pre", Rational( 1, 2 ) );
    //peopleScalePre.registerProvider( peopleLimiter );
    //app.addThread( &peopleScalePre );

    // people detect for people
    ShapeDetector peopleDetector( "people-detector", "dlib_pedestrian_detector.svm", ShapeDetector::OZ_SHAPE_MARKUP_OUTLINE );
    peopleDetector.registerProvider( people );
    app.addThread( &peopleDetector );

    // Scale video down
    ImageScale peopleScalePost( "people-scale-post", Rational( 1, 2 ) );
    peopleScalePost.registerProvider( peopleDetector );
    app.addThread( &peopleScalePost );

    // Turn black and white
    VideoFilter peopleFilter( "people-filter", "hue=s=0, scale=iw/2:-1" );
    //VideoFilter peopleFilter( "people-filter", "scale=iw/2:-1" );
    peopleFilter.registerProvider( peopleScalePost );
    //app.addThread( &peopleFilter );

    // Let's make a mux/stitched handler for traffic and people and its debugs
    MatrixVideo trafficMatrix( "traffic-matrix", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    trafficMatrix.registerProvider( *trafficDetector.compImageSlave() );
    trafficMatrix.registerProvider( *trafficDetector.refImageSlave() );
    trafficMatrix.registerProvider( *trafficDetector.varImageSlave() );
    trafficMatrix.registerProvider( *trafficDetector.deltaImageSlave() );
    //app.addThread( &trafficMatrix );

    Listener listener;
    app.addThread( &listener );

    HttpController httpController( "watch", 9292 );
    //httpController.addStream("watch",traffic);
    httpController.addStream( "watch", people );

    //httpController.addStream( "detect", trafficDetector );
    httpController.addStream( "detect", peopleDetector );
    httpController.addStream( "detect", peopleScalePost );
    httpController.addStream( "detect", peopleFilter );

    //httpController.addStream( "debug", trafficMatrix );
    
    listener.addController( &httpController );

    app.run();
}

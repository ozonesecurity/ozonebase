#include "../base/ozApp.h"
#include "../base/ozListener.h"
#include "../providers/ozAVInput.h"
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
    AVInput traffic( "traffic", "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
    Options peopleOptions;
    peopleOptions.add( "realtime", true );
    peopleOptions.add( "loop", true );
    AVInput people( "people", "face-input.mp4", peopleOptions );
    app.addThread( &traffic );
    app.addThread( &people );

    // motion detect for traffic
    Options motionOptions;
    motionOptions.load( "OZ_OPT_MOTION_" );
    motionOptions.dump();
    MotionDetector trafficDetector( traffic, motionOptions );
    app.addThread( &trafficDetector );

    // rate limiter for people detector
    RateLimiter peopleLimiter( 5, true, people );
    app.addThread( &peopleLimiter );

    // Scale video up
    //ImageScale peopleScalePre( Rational( 1, 2 ), peopleLimiter );
    //app.addThread( &peopleScalePre );

    // people detect for people
    //ShapeDetector peopleDetector( "dlib_pedestrian_detector.svm", ShapeDetector::OZ_SHAPE_MARKUP_OUTLINE, peopleLimiter );
    Options faceOptions;
    faceOptions.set( "method", "hog" );
    faceOptions.set( "dataFile", "shape_predictor_68_face_landmarks.dat" );
    faceOptions.set( "markup", FaceDetector::OZ_FACE_MARKUP_ALL );
    FaceDetector peopleDetector( peopleLimiter, faceOptions );
    app.addThread( &peopleDetector );

    // Scale video down
    //ImageScale peopleScalePost( Rational( 1, 2 ), peopleDetector );
    //app.addThread( &peopleScalePost );

    // Turn black and white
    //VideoFilter peopleFilter( "hue=s=0, scale=iw/2:-1", people );
    //VideoFilter peopleFilter( "people-filter", "scale=iw/2:-1" );
    //peopleFilter.registerProvider( people );
    //app.addThread( &peopleFilter );

    // Let's make a mux/stitched handler for traffic and people and its debugs
    MatrixVideo trafficMatrix( "traffic-matrix", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    trafficMatrix.registerProvider( *trafficDetector.compImageSlave() );
    trafficMatrix.registerProvider( *trafficDetector.refImageSlave() );
    trafficMatrix.registerProvider( *trafficDetector.varImageSlave() );
    trafficMatrix.registerProvider( *trafficDetector.deltaImageSlave() );
    app.addThread( &trafficMatrix );

    Listener listener;
    app.addThread( &listener );

    HttpController httpController( "watch", 9292 );
    httpController.addStream( "watch",traffic );
    httpController.addStream( "watch", people );

    httpController.addStream( "detect", trafficDetector );
    httpController.addStream( "detect", peopleDetector );
    //httpController.addStream( "filter", peopleFilter );

    httpController.addStream( "debug", trafficMatrix );
    
    listener.addController( &httpController );
    app.run();
}

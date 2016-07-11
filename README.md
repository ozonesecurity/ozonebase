# Ozone - state of the art framework for developing security and surveillance solutions.


[![Build Status](https://travis-ci.org/ozonesecurity/ozonebase.svg?branch=master)](https://travis-ci.org/ozonesecurity/ozonebase)

### First an example:

**Problem Statement 1: How about we tap into two public RTSP cameras, and display them on a browser (no RTSP support), run live motion detection on them, show motion frames and, yeah, while we are at it, stitch the two camera images and their live debug frames into a 4x4 muxed image?**

**Problem Statement 2: Oh yeah, in 10 minutes.**


**Answer: Tap on the image below to see a recording of a live video**

[![ozone server video](http://img.youtube.com/vi/Ic2HXUjxRnU/0.jpg)](http://www.youtube.com/watch?v=Ic2HXUjxRnU "ozone server example")

What's happening here is we wrote a simple server and client app. The server app connects to two traffic cameras, converts RTSP to MJPEG so it can show on the browser. It also creates motion detection streams and a stitched matrix frame to show you it rocks. And in less that 22 lines of core code. 



The client (HTML) code that renders the images above:
Full code [here](https://github.com/ozonesecurity/ozonebase/blob/master/server/src/examples/starter_example.html)

```
<!-- REPLACE THE IP ADDRESS WITH YOUR SERVER IP -->
<html>
<body>
  <table>
    <h1>Example of the amazing simplicity of ozone server framework<h1>
    <h5>Source code:<a href="https://github.com/ozonesecurity/ozonebase/blob/master/server/src/examples/starter_example.cpp">here</a></h5>
    <th>Live Feeds, 2 traffic cameras</th>
     <tr>
        <!-- live frames -->
        <td>
            <img src="http://192.168.1.224:9292/watchcam1/cam1" />
        </td>
        <td>
            <img src="http://192.168.1.224:9292/watchcam2/cam2" />
        </td>
     </tr>
     <!-- debug frames -->
     <tr>
        <td>
          Modect on cam1:<br/>
          <img src="http://192.168.1.224:9292/debug/modectcam1" />
        </td>
        <td>
          Modect on cam2:<br/>
          <img src="http://192.168.1.224:9292/debug/modectcam2" />
        </td>
    </tr>
  </table>
 
<h2> Muxed 4x4 stream of camera 1 and 2 and its debug frames</h2>
 <img src="http://192.168.1.224:9292/debug/matrixcammux" />


</body>
</html>
```


The servercode that generates the images above:
Full code [here](https://github.com/ozonesecurity/ozonebase/blob/master/server/src/examples/starter_example.cpp)


```
	Application app;

   	// Two RTSP sources 
    NetworkAVInput cam1( "cam1", "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
    NetworkAVInput cam2("cam2","rtsp://170.93.143.139:1935/rtplive/e0ffa81e00a200ab0050fa36c4235c0a");
    app.addThread( &cam1 );
    app.addThread( &cam2 );

	// motion detect for cam1
	MotionDetector motionDetector1( "modectcam1" );
  	motionDetector1.registerProvider( cam1 );
   	app.addThread( &motionDetector1 );

	// motion detect for cam1
	MotionDetector motionDetector2( "modectcam2" );
  	motionDetector2.registerProvider( cam2 );
   	app.addThread( &motionDetector2 );

	// Let's make a mux/stitched handler for cam1 and cam2 and its debugs
	MatrixVideo matrixVideo( "matrixcammux", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
   	matrixVideo.registerProvider( cam1 );
   	matrixVideo.registerProvider( *motionDetector1.deltaImageSlave() );
   	matrixVideo.registerProvider( cam2 );
   	matrixVideo.registerProvider( *motionDetector2.deltaImageSlave() );
   	app.addThread( &matrixVideo );

	Listener listener;
    app.addThread( &listener );

    HttpController httpController( "watch", 9292 );
    httpController.addStream("watchcam1",cam1);
    httpController.addStream("watchcam2",cam2);

	httpController.addStream( "file", cam1 );
   	httpController.addStream( "debug", SlaveVideo::cClass() );
   	httpController.addStream( "debug", matrixVideo );
   	httpController.addStream( "debug", motionDetector1 );
   	httpController.addStream( "debug", motionDetector2 );
	
    listener.addController( &httpController );

    app.run();
```


## Excited? Want to know more?

Ozone is is rockstar framework to make development of custom security and surveillance products much easier than today.
Completely re-written and more modern in architecture. Ozone is created by the original developer of ZoneMinder, Phil Coombes.

Ozone comprises of:

* server - base framework for developing security and surveilllance servers (like ZoneMinder, for example)
* client - base framework for developing mobile apps for remote control (like zmNinja, for example)
* examples - reference implementations to get  you started


## What is the goal?

Ozone a more modern take and has been re-architected to serve as a UI-less framework at its core. We expect this to be the core framework for developing full fledged apps on top. Our hope is that this will form the base of many security solutions - developers can use this library to accelerate their own vision of security software. Over time, we will also offer our own fully running UI enabled version running over NodeJS/Angular.

## Intended audience

We expect OEMs, ISVs and 3rd party developers to use this library to build their own solutons powered by us. It is not an end consumer product, unlike V1.

As of today, there is no roadmap to merge this with version 1. Version 1 has a seperate goal from Version 2 and we think its best to leave it that way for now.

## Who is developing and maintaining Ozone?
The Ozonebase server framework is developed by [Philip Coombes](https://github.com/web2wire), the original developer of ZoneMinder.
The codebase will be maintained by Phil and [Pliable Pixels](https://github.com/pliablepixels) and other key contributors who will be added soon.

Ozonebase part of a solution suite being developed by [Ozone Networks](http://ozone.network). The full solution suite will eventually include a server framework (this code), a mobile framework for white-labelled apps and a reference solution using them.

Ozonebase is dual-licensed.
Please refer to the [LICENSING](LICENSE.md) file for details.

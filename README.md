# Ozone - state of the art framework for developing security and surveillance solutions for the Home, Commercial and more.


### First an example:

**Problem Statement 1: How about we tap into two public RTSP cameras, and display them on a browser (no RTSP support), run live motion detection on them, show motion frames and, yeah, while we are at it, stitch the two camera images and their live debug frames into a 4x4 muxed image?**

**Problem Statement 2: Oh yeah, in 10 minutes.**


**Answer: The output**

<iframe width="420" height="315" src="https://www.youtube.com/embed/Ic2HXUjxRnU" frameborder="0" allowfullscreen></iframe>



[This code](https://github.com/ozonesecurity/ozonebase/blob/master/server/src/examples/starter_example.cpp), produces this output:


```
Application app;

    NetworkAVInput input( "input", "rtsp://170.93.143.139:1935/rtplive/0b01b57900060075004d823633235daa" );
    app.addThread( &input );

	MotionDetector motionDetector( "modect" );
  	motionDetector.registerProvider( input );
   	app.addThread( &motionDetector );

	QuadVideo quadVideo( "quad", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
   	quadVideo.registerProvider( *motionDetector.refImageSlave() );
   	quadVideo.registerProvider( *motionDetector.compImageSlave() );
   	quadVideo.registerProvider( *motionDetector.deltaImageSlave() );
   	quadVideo.registerProvider( *motionDetector.varImageSlave() );
   	app.addThread( &quadVideo );

    Listener listener;
    app.addThread( &listener );

    HttpController httpController( "watch", 9292 );
    httpController.addStream("watch",input);

	httpController.addStream( "file", input );
   	httpController.addStream( "debug", SlaveVideo::cClass() );
   	httpController.addStream( "debug", quadVideo );
   	httpController.addStream( "debug", motionDetector );
	
    listener.addController( &httpController );


    app.run();
```


Ozone is the next generation of ZoneMinder.
Completely re-written and more modern in architecture. While [Version 1](https://github.com/zoneminder) is still used by many, we felt its time to take a fresh approach as a lot of the code is legacy and hard to extend, not to mention that over the last 10 years, technology has evolved significantly.
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

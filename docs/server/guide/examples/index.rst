Examples
========
The `examples <https://github.com/ozonesecurity/ozonebase/tree/master/server/src/examples>`_ directory contain a series of simple examples that help you get started. A good place to start is `starter_example.cpp/html`. In general, the code you write with ozone is the 'backend' code. It doesn't start a UI. Its job is to connect to a video source and process it. So in the case of `starter_example`: 

* `starter_example.cpp` is a process  that connects to two video sources: first, it connects to a public traffic camera and second, it tries to load a file called "face-input".mp4. Once it loads the two files, it does motion detect on one, and does a face detection on another. It also instantiates a http server that when connected to should display 4 windows - two video windows + 1 motion detection  window on the traffic camera + 1 face detection window on "face-input.mp4"

* Make sure the start_example process runs successfully - look at the logs to make sure its not erroring (see source code on how to see logs)

* `start_example.html` is the browser HTML that you can open to display the windows - open it in a browser, but before you do, change its `$scope.baseurl` IP to the IP where you are running starter_example process.

* Please read through the source code to understand more - this framework is really meant for developers.

You'll notice that this example won't run - it will complain that it can't find "shape_predictor". Read `this <https://github.com/ozonesecurity/ozonebase/tree/master/server/src/examples/models>`_

You might also want to know where "face-input.mp4" is. Its any video file you download/have that has faces (easily understandable) to detect.

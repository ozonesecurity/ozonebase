/** @defgroup Consumers
 *
 * These will consume frames and do something with them (e.g. save to a filesystem).
 *
 */


/** @defgroup Providers
*
* These will generate frames from some input source (like RTSP, local, etc.)
*
*/

/** @defgroup Processors
*
* These will both consume and (usually) provide frames. For instance a motion detection component consume video frames and then outputs them potentially with additional information details motion detected etc.
*
*/

/** @defgroup Application
*
* The main application object that keeps track of various component threads
*/

/** @defgroup Utilities
*
* Various utility classes
*/

/*! \mainpage Welcome
 
  <a href="http://ozone.network">&lt;oZone</a>
  <br/>

  Please refer to the <a href="http://ozone-framework.readthedocs.io/en/latest/">User Guide</a> for an introduction to the architecture.

  At a high level, the oZone server framework is distributed into the following key components:<br/>
  
  <ul>
  	<li>\ref Providers</li>
	<li>\ref Consumers</li>
	<li>\ref Processors</li>
  </ul>

  <p>Providers are components that generate frames (such as IP cameras (NetworkAVInput, RawH264Input), movie files (LocalVideoInput) etc)</p>
  <p>Consumers are components that consume those frames and do something with it (example convert Provider frames to a movie file)</p>
  <p>Processors are components that both "consume" frames from a Provider and "provide" processed frames to other consumers or processors. For example, MotionDetector is a Processor that consumes frames from say a video camera provider, performs motion detection on frames and outputs only frames with motion in them for others to connect to (such as EventRecorder)</p>
 
<p>
In addition to the core components above, there are other components that form an integral part in connecting the dots:

<ul>
<li>Controller – A controller is required to handle requests for a particular protocol, e.g. HTTPController, RtspController. Controllers register with the listener to allow it to route requests appropriately.</li>
<li>\ref Protocols – A protocol is required to handle the low level communication for a controller.</li>
<li>\ref Session – A protocol may need a session component to manage persistence for a connection or other session based behaviour.</li>
<li>\ref Stream – A provider can add a stream to a controller to make specific video or audio frames available over the network.</li>
<li>\ref Listener – This is a general component that is required to handle any incoming network connection.</li>
</ul>

</p>

<p>Please browse the <a href="modules.html">modules</a> category on the sidebar to read more about the various classes</p>

<p>A visual layout of how these components interact is shown below:
<br/>
<img src="../public/images/ozone-mainpage-class-ref.png" width="500px" style="float:left">
</p>

 */

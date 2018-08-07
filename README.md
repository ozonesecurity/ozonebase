
# Notice

**oZone is now MIT licensed**

Due to a change in priority in our personal lives, ozone is not actively maintained at the moment. It's pretty stable and well documented but we are not active in support/patches. That being said, if this project is useful for you, feel free to contribute/PR. This status may change if either Phil or I get freed up again.

# Ozone -  an easy to use platform for Video Innovation

[![Build Status](https://travis-ci.org/ozonesecurity/ozonebase.svg?branch=master)](https://travis-ci.org/ozonesecurity/ozonebase)
[![Join Slack](https://github.com/ozonesecurity/ozonebase/blob/master/img/slacksm.png?raw=true)](https://ozone.herokuapp.com)


Read/Follow our [Medium publication](https://medium.com/ozone-security) for interesting applications of oZone

Ozone is a modern and fully component based approach to tying together some of the best opensource libraries in the world in the areas of image manipulation, recognition and deep learning under an abstract and easy interface. oZone makes it very easy to import data from _any_ video source (live/recorded/file/web/proprietary) and apply intelligent decisions on top that analyze and react to the data contained in the frames.

Ozone is not a "Video Analytics" solution. There are many companies doing good work in this area. We are the 'unifying platform' underneath that makes it easy for you to take the best 3rd party libraries (many of which we already include) that server your purpose and build your app without learning new interfaces/languages or approach.

oZone already provides key components and functionality like:
* **Any source** - we support many video formats already. Don't see yours? Add a Provider for it.
* **Recording service** - that can automatically create videos for events you define
* **Detection** - Motion Detection, Face Detection, Shape Detection (dlib). Have your own amazing detection algorithm? Write a Processor for it.
* **Recognition** - License plate recognition, Tensor flow (image recognition - in progress) or write your own
* **Chaining** - chain multiple components to incrementally add functionality (example, chain Shape Detection to Motion Detection to only reach if there is Motion AND it matches the shapes you are looking for)
* **Arbitrary Image Filters** - for deep diving into image transformations (histograms/edge detection/scaling/time stamping/etc.)
* **Trigger framework** - allows you to combine external triggers (example temperature/location/etc) with image triggers for complex processing

## Quick start
[Web](http://ozone.network) & [Blog](https://medium.com/ozone-security)

[Key Concepts](http://ozone-framework.readthedocs.io/en/latest/index.html)

[C++ API](http://ozone.network/apidocs/index.html)


## Who is developing and maintaining Ozone?
The Ozonebase server framework was initially developed by [Philip Coombes](https://github.com/web2wire), the original developer of ZoneMinder. The codebase will be maintained and extended by Phil and [Pliable Pixels](https://github.com/pliablepixels) 



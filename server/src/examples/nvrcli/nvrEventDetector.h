
#ifndef NVR_EVENT_DETECTOR_H
#define NVR_EVENT_DETECTOR_H

#include "ozone.h"
#include <base/ozFeedConsumer.h>
#include <libgen/libgenThread.h>

#include <base/ozMotionFrame.h>

#include <deque>
#include <functional>

// override relevant methods
class EventDetector: public EventRecorder { 
public:
	EventDetector (const std::string &name, const std::string &location, NetworkAVInput *cam):
		mCam(cam),
		EventRecorder(name,location)
	{}
	
protected:
	 NetworkAVInput *mCam;
	int run();
	bool processFrame(FramePtr frame);
	
    
};

#endif // NVR_EVENT_DETECTOR_H


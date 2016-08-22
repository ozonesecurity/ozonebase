#include <base/ozApp.h>
#include <base/ozListener.h>

#include <providers/ozNetworkAVInput.h>
#include <providers/ozLocalFileInput.h>
#include <providers/ozLocalVideoInput.h>


#include <consumers/ozEventRecorder.h>
#include <consumers/ozMovieFileOutput.h>
#include <consumers/ozVideoRecorder.h>


#include <processors/ozMotionDetector.h>
#include <processors/ozImageConvert.h>
#include <processors/ozFaceDetector.h>
#include <processors/ozMatrixVideo.h>
#include <processors/ozRateLimiter.h>
#include <processors/ozImageTimestamper.h>

#include <protocols/ozHttpController.h>

#include <libgen/libgenDebug.h>

class Detector:public VideoConsumer, public VideoProvider, public Thread
{};

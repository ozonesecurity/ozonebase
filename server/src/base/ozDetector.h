#ifndef OZ_DETECTOR_H
#define OZ_DETECTOR_H

#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"
#include "../libimg/libimgImage.h"
#include "../providers/ozSlaveVideo.h"

class Detector: public virtual VideoConsumer, public virtual VideoProvider, public virtual Thread
{
  CLASSID(Detector);  
};

#endif // OZ_DETECTOR_H

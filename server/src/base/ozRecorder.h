#ifndef OZ_RECORDER_H
#define OZ_RECORDER_H

#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"
#include "../libimg/libimgImage.h"
#include "../providers/ozSlaveVideo.h"

class Recorder: public virtual VideoConsumer, public virtual DataProvider, public virtual Thread
{
  CLASSID(Recorder);  
};

#endif // OZ_RECORDER_H

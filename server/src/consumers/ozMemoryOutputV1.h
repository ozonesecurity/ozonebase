/** @addtogroup Consumers */
/*@{*/
#ifndef OZ_MEMORY_OUTPUT_V1_H
#define OZ_MEMORY_OUTPUT_V1_H

#include "../base/ozFeedConsumer.h"
#include "../base/ozMemoryIOV1.h"
#include "../libgen/libgenThread.h"

class Image;

///
/// Consumer class that writes received video frames to shared memory. These frames
/// can then be accessed by processes using the MemoryInput class to retrieve the
/// frames. This allow easy reproduction of the traditional ZoneMinder, zmc, zma and
/// zms behaviour.
///
class MemoryOutputV1 : public VideoConsumer, public MemoryIOV1, public Thread
{
CLASSID(MemoryOutputV1);

protected:
    uint64_t    mImageCount;

protected:
    int run();
    bool storeFrame( const FramePtr &frame );

public:
    MemoryOutputV1( const std::string &name, const std::string &location, int memoryKey );
    ~MemoryOutputV1();
};

#endif // OZ_MEMORY_OUTPUT_V1_H
/*@}*/

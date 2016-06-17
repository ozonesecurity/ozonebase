/** @addtogroup Consumers */
/*@{*/
#ifndef ZM_MEMORY_OUTPUT_H
#define ZM_MEMORY_OUTPUT_H

#include "../base/ozFeedConsumer.h"
#include "../base/ozMemoryIO.h"
#include "../libgen/libgenThread.h"

class Image;

///
/// Consumer class that writes received video frames to shared memory. These frames
/// can then be accessed by processes using the MemoryInput class to retrieve the
/// frames. This allow easy reproduction of the traditional ZoneMinder, ozc, oza and
/// ozs behaviour.
///
class MemoryOutput : public VideoConsumer, public MemoryIO, public Thread
{
CLASSID(MemoryOutput);

protected:
    uint64_t            mImageCount;

protected:
    int run();
    bool storeFrame( FramePtr frame );

public:
    MemoryOutput( const std::string &name, const std::string &location, int memoryKey );
    ~MemoryOutput();
};

#endif // ZM_MEMORY_OUTPUT_H
/*@}*/

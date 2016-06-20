/** @addtogroup Providers */
/*@{*/


#ifndef OZ_MEMORY_INPUT_H
#define OZ_MEMORY_INPUT_H

#include "../base/ozFeedProvider.h"
#include "../base/ozMemoryIO.h"
#include "../libgen/libgenThread.h"

class Image;

///
/// Class representing video provider which access frames written to shared memory by
/// MemoryOutput class. Can be used reproduce traditional ZoneMinder ozc/oza relationship
/// and behaviour.
///
class MemoryInput : public VideoProvider, public MemoryIO, public Thread
{
CLASSID(MemoryInput);

protected:
    PixelFormat mImageFormat;
    uint16_t    mImageWidth;
    uint16_t    mImageHeight;
    FrameRate	mFrameRate;

protected:
    int run();
    const FeedFrame *loadFrame();

public:
    MemoryInput( const std::string &name, const std::string &location, int memoryKey );
    ~MemoryInput();

public:
    PixelFormat pixelFormat() const { return( mImageFormat ); }
    uint16_t width() const { return( mImageWidth ); }
    uint16_t height() const { return( mImageHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }
};

#endif // OZ_MEMORY_INPUT_H


/*@}*/

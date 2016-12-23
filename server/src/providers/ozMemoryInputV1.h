/** @addtogroup Providers */
/*@{*/


#ifndef OZ_MEMORY_INPUT_V1_H
#define OZ_MEMORY_INPUT_V1_H

#include "../base/ozFeedProvider.h"
#include "../base/ozMemoryIOV1.h"
#include "../libgen/libgenThread.h"

class Image;

///
/// Class representing video provider which access frames written to shared memory by
/// MemoryOutput class. Can be used reproduce traditional ZoneMinder ozc/oza relationship
/// and behaviour.
///
class MemoryInputV1 : public VideoProvider, public MemoryIOV1, public Thread
{
CLASSID(MemoryInputV1);

protected:
    int         mImageCount;
    PixelFormat mPixelFormat;   ///< FFmpeg equivalent image format
    uint16_t    mImageWidth;    ///< Requested video width, applies to all channels
    uint16_t    mImageHeight;   ///< Requested video height, applies to all channels
    FrameRate   mFrameRate;     ///< Requested frame rate

protected:
    int run();
    const FeedFrame *loadFrame();

public:
    MemoryInputV1( const std::string &name,
                   const std::string &location,
                   int memoryKey,
                   int imageCount,
                   PixelFormat pixelFormat,
                   uint16_t imageWidth,
                   uint16_t imageHeight
    );
    ~MemoryInputV1();

public:
    PixelFormat pixelFormat() const { return( mPixelFormat ); }
    uint16_t width() const { return( mImageWidth ); }
    uint16_t height() const { return( mImageHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }

};

#endif // OZ_MEMORY_INPUT_V1_H

/*@}*/

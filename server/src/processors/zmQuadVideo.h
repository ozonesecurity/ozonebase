/** @addtogroup Processors */
/*@{*/


#ifndef ZM_QUAD_VIDEO_H
#define ZM_QUAD_VIDEO_H

#include "../base/zmFeedBase.h"
#include "../base/zmFeedProvider.h"
#include "../base/zmFeedConsumer.h"

///
/// Processor that amalgamates up to four video streams into a 2x2 quad video matrix. Can easily
/// be extended for other geometries but the overall image size must be exactly divisble by the number
/// of tiles. Bit of a kludge implementation, a more flexible variant should be possible using ffmpeg
/// filters.
///
class QuadVideo : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(QuadVideo);

private:
    typedef std::vector<FeedProvider *>   ProviderList;

private:
    PixelFormat     mPixelFormat;
    int             mWidth;
    int             mHeight;
    FrameRate       mFrameRate;
    ProviderList    mProviderList;

    int             mTilesX;
    int             mTilesY;
    int             mTiles;

    AVFrame         **mInterFrames;
    struct SwsContext **mConvertContexts;

public:
    QuadVideo( const std::string &name, PixelFormat pixelFormat, int width, int height, FrameRate frameRate, int xTiles=2, int yTiles=2 );
    ~QuadVideo();

    PixelFormat pixelFormat() const { return( mPixelFormat ); }
    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }

    bool registerProvider( FeedProvider &provider, const FeedLink &link=gQueuedFeedLink );
    bool deregisterProvider( FeedProvider &provider, bool reciprocate=true );
             
protected:
    int run();
};

#endif // ZM_QUAD_VIDEO_H


/*@}*/

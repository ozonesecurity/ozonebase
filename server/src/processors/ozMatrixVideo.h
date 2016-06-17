/** @addtogroup Processors */
/*@{*/


#ifndef OZ_MATRIX_VIDEO_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

///
/// Processor that amalgamates up to four video streams into a 2x2 matrix video matrix. Can easily
/// be extended for other geometries but the overall image size must be exactly divisble by the number
/// of tiles. Bit of a kludge implementation, a more flexible variant should be possible using ffmpeg
/// filters.
///
class MatrixVideo : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(MatrixVideo);

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
    MatrixVideo( const std::string &name, PixelFormat pixelFormat, int width, int height, FrameRate frameRate, int xTiles=2, int yTiles=2 );
    ~MatrixVideo();

    PixelFormat pixelFormat() const { return( mPixelFormat ); }
    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }

    bool registerProvider( FeedProvider &provider, const FeedLink &link=gQueuedFeedLink );
    bool deregisterProvider( FeedProvider &provider, bool reciprocate=true );
             
protected:
    int run();
};

#endif // OZ_MATRIX_VIDEO_H


/*@}*/

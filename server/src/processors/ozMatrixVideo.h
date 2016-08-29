/** @addtogroup Processors */
/*@{*/


#ifndef OZ_MATRIX_VIDEO_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

///
/// Processor that amalgamates video streams into a mxn matrix video matrix. Can easily
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
/**
* @brief 
*
* @param name Instance name
* @param pixelFormat format of image (PIX_FMT_YUV420 or other AVPixelFormat types)
* @param widtha Total width of matrix
* @param height Total height of matrix
* @param frameRate  framerate per second to update the matrix feeds
* @param xTiles m (of mxn tiles)
* @param yTiles n (of mxn tiles)

\code
    MatrixVideo matrixVideo( "matrixcammux", PIX_FMT_YUV420P, 640, 480, FrameRate( 1, 10 ), 2, 2 );
    matrixVideo.registerProvider( cam1 );
    matrixVideo.registerProvider( *motionDetector1.deltaImageSlave() );
    matrixVideo.registerProvider( cam2 );
    matrixVideo.registerProvider( *motionDetector2.deltaImageSlave() );
    app.addThread( &matrixVideo );
\endcode



*/
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

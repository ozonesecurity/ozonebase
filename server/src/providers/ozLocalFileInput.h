/** @addtogroup Providers */
/*@{*/


#ifndef OZ_LOCAL_FILE_INPUT_H
#define OZ_LOCAL_FILE_INPUT_H

#include "../base/ozFeedProvider.h"
#include "../base/ozFeedFrame.h"
#include "../libgen/libgenThread.h"

///
/// Class used for constructing streams from individual images
///
class LocalFileInput : public VideoProvider, public Thread
{
CLASSID(LocalFileInput);

private:
    std::string     mPattern;       ///< Pattern to match files against
    int             mWidth;         ///< Requested video width, applies to all channels
    int             mHeight;        ///< Requested video height, applies to all channels
    FrameRate       mFrameRate;     ///< Requested frame rate
    PixelFormat     mPixelFormat;   ///< FFmpeg equivalent image format

public:
    LocalFileInput( const std::string &name, const std::string &pattern, const FrameRate &frameRate );
    ~LocalFileInput();

    uint16_t width() const
    {
        return( mWidth );
    }
    uint16_t height() const
    {
        return( mHeight );
    }
    PixelFormat pixelFormat() const
    {
        return( mPixelFormat );
    }
    FrameRate frameRate() const
    {
        return( mFrameRate );
    }

protected:
    int run();
};

#endif // OZ_LOCAL_FILE_INPUT_H


/*@}*/

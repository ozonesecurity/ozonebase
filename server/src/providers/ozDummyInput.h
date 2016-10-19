/** @addtogroup Providers */
/*@{*/

#ifndef OZ_DUMMY_INPUT_H
#define OZ_DUMMY_INPUT_H

#include "../base/ozFeedFrame.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozOptions.h"
#include "../libgen/libgenThread.h"

///
/// Class using ffmpeg libavcodec functions to read and decode video and audio from any
/// supported network source.
///
class DummyInput : public VideoProvider, public Thread
{
CLASSID(DummyInput);

private:
	Options			mOptions;       ///< Various options
    int             mWidth;         ///< Requested video width, applies to all channels
    int             mHeight;        ///< Requested video height, applies to all channels
    AVPixelFormat   mPixelFormat;   ///< FFmpeg equivalent image format
    FrameRate       mFrameRate;     ///< Requested frame rate
    std::string     mColour;        ///< Frame background colour
    std::string     mText;          ///< Text to show in the frame
    uint16_t        mTextSize;      ///< Size of text 
    std::string     mTextColour;    ///< Colour of text 

public:
    DummyInput( const std::string &name, const Options &options=gNullOptions );
    ~DummyInput();

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

#endif // OZ_DUMMY_INPUT_H


/*@}*/

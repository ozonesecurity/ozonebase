/** @addtogroup Providers */
/*@{*/


#ifndef OZ_LOCAL_VIDEO_INPUT_H
#define OZ_LOCAL_VIDEO_INPUT_H

#include "../base/ozFeedProvider.h"
#include "../base/ozFeedFrame.h"
#include "../libgen/libgenThread.h"

///
/// Class representing analog video provider accessed via /dev/videoX files.
/// Uses ffmpeg to supply video frames.
///
class LocalVideoInput : public VideoProvider, public Thread
{
CLASSID(LocalVideoInput);

private:
    std::string mSource;            ///< String containing path to video device
    AVCodecContext *mCodecContext;  ///< Ffmpeg codec context containg details of video source
    AVStream *mStream;              ///< Ffmpeg structure containg details of video stream

public:
    LocalVideoInput( const std::string &name, const std::string &source );
    ~LocalVideoInput();

    //const FeedFrame *getFrame() const;
    const AVCodecContext *codecContext() const { return( mCodecContext ); }

    uint16_t width() const
    {
        return( mCodecContext->width );
    }
    uint16_t height() const
    {
        return( mCodecContext->height );
    }
    PixelFormat pixelFormat() const
    {
        return( mCodecContext->pix_fmt );
    }
    FrameRate frameRate() const
    {
        return( mStream ? mStream->r_frame_rate : FrameRate(0,0) );
    }
    TimeBase videoTimeBase() const
    {
        return( mCodecContext ? mCodecContext->time_base : TimeBase(0,0) );
    }

protected:
    int run();
};

#endif // OZ_LOCAL_VIDEO_INPUT_H


/*@}*/

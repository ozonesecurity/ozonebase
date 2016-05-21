#ifndef ZM_LOCAL_VIDEO_INPUT_H
#define ZM_LOCAL_VIDEO_INPUT_H

#include "../zmFeedProvider.h"
#include "../zmFeedFrame.h"
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
        return( mStream?mStream->r_frame_rate:FrameRate(0,0) );
    }

protected:
    int run();
};

#endif // ZM_LOCAL_VIDEO_INPUT_H

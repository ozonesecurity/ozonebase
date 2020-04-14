/** @addtogroup Providers */
/*@{*/


#ifndef OZ_REMOTE_VIDEO_INPUT_H
#define OZ_REMOTE_VIDEO_INPUT_H

#include "../base/ozFeedFrame.h"
#include "../base/ozFeedProvider.h"
#include "../libgen/libgenThread.h"

///
/// Class used for network video using ffmpeg libavcodec libraries. Superseded by NetworkAVInput which also
/// handles audio.
///
class RemoteVideoInput : public VideoProvider, public Thread
{
CLASSID(RemoteVideoInput);

private:
    std::string mSource;
    std::string mFormat;
    AVCodecContext *mCodecContext;
    uint64_t    mBaseTimestamp;     ///< Remote streams tend to be timebased on time at stream start

public:
    RemoteVideoInput( const std::string &name, const std::string &source, const std::string &format="" );
    ~RemoteVideoInput();

    const FeedFrame *getFrame() const;
    const AVCodecContext *codecContext() const { return( mCodecContext ); }

    uint16_t width() const
    {
        return( mCodecContext->width );
    }
    uint16_t height() const
    {
        return( mCodecContext->height );
    }
    AVPixelFormat pixelFormat() const
    {
        return( mCodecContext->pix_fmt );
    }
    FrameRate frameRate() const
    {
        return( mCodecContext->time_base );
    }

protected:
    int run();
};

#endif // OZ_REMOTE_VIDEO_INPUT_H


/*@}*/

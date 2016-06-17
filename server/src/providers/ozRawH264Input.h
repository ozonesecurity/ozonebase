/** @addtogroup Providers */
/*@{*/


#ifndef ZM_RAW_H264_INPUT_H
#define ZM_RAW_H264_INPUT_H

#include "../base/ozFeedFrame.h"
#include "../base/ozFeedProvider.h"
#include "../libgen/libgenThread.h"

///
/// Class using ffmpeg libavcodec functions to read and decode video and audio from any
/// supported network source.
///
class RawH264Input : public VideoProvider, public Thread
{
CLASSID(RawH264Input);

private:
    std::string     mSource;                ///< String containing address of network media
    std::string     mFormat;                ///< String containing hint regarding format of network media
    AVCodecContext  *mVideoCodecContext;    ///< Structure containing details of the received video, if present
    AVCodecContext  *mAudioCodecContext;    ///< Structure containing details of the received audio, if present
    AVStream        *mVideoStream;          ///< Structure containing details of the received video stream, if present
    AVStream        *mAudioStream;          ///< Structure containing details of the received audio stream, if present
    uint64_t        mBaseTimestamp;         ///< Remote streams tend to be timebased on time at stream start,
                                        ///< this is the initial timestamp for reference

public:
    RawH264Input( const std::string &name, const std::string &source, const std::string &format="" );
    ~RawH264Input();

    const AVCodecContext *videoCodecContext() const { return( mVideoCodecContext ); }
    const AVCodecContext *audioCodecContext() const { return( mAudioCodecContext ); }

    PixelFormat pixelFormat() const
    {
        return( mVideoCodecContext->pix_fmt );
    }
    uint16_t width() const
    {
        return( mVideoCodecContext->width );
    }
    uint16_t height() const
    {
        return( mVideoCodecContext->height );
    }
    FrameRate frameRate() const
    {
        if ( mVideoStream )
            return( mVideoStream->r_frame_rate );
        return( 0 );
    }
    AVSampleFormat sampleFormat() const
    {
        return( mAudioCodecContext->sample_fmt );
    }
    uint32_t sampleRate() const
    {
        return( mAudioCodecContext->sample_rate );
    }
    uint8_t channels() const
    {
        return( mAudioCodecContext->channels );
    }
    uint16_t samples() const
    {
        return( mAudioCodecContext->frame_size );   // Not sure about this
    }

protected:
    int run();
};

#endif // ZM_RAW_H264_INPUT_H


/*@}*/

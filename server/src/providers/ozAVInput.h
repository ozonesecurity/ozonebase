/** @addtogroup Providers */
/*@{*/

#ifndef OZ_AV_INPUT_H
#define OZ_AV_INPUT_H

#include "../base/ozFeedFrame.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozOptions.h"
#include "../libgen/libgenThread.h"

///
/// Class using ffmpeg libavcodec functions to read and decode video and audio from any
/// supported network source.
///
class AVInput : public AudioVideoProvider, public Thread
{
CLASSID(AVInput);

private:
    std::string     mSource;                ///< String containing address of network media
	Options			mOptions;               ///< Various options
    AVCodecContext  *mVideoCodecContext;    ///< Structure containing details of the received video, if present
    AVCodecContext  *mAudioCodecContext;    ///< Structure containing details of the received audio, if present
    AVStream        *mVideoStream;          ///< Structure containing details of the received video stream, if present
    AVStream        *mAudioStream;          ///< Structure containing details of the received audio stream, if present
    int             mVideoStreamId;
    int             mAudioStreamId;
    AVFrame         *mVideoFrame;
    AVFrame         *mAudioFrame;
    ByteBuffer      mVideoFrameBuffer;
    ByteBuffer      mAudioFrameBuffer;
    uint64_t        mBaseTimestamp;         ///< Remote streams tend to be timebased on time at stream start,
                                            ///< this is the initial timestamp for reference
    uint64_t        mLastTimestamp;         ///< Remote streams tend to be timebased on time at stream start,
    bool            mRealtime;

public:
/**
* @brief 
*
* @param name instance name
* @param source AV source (example "rtsp://foo", "/dev/video0", "/tmp/test.mp4" etc)
* @param options list of options that you can use to configure the input with

\code
AVOptions avOptions;
avOptions.add("realtime",true); // respects FPS if video is not live
avOptions.add("loop",true); // keeps looping video if file

nvrcam.cam = new AVInput ( name, source,avOptions );

\endcode
*/
    AVInput( const std::string &name, const std::string &source, const Options &options=gNullOptions );
    ~AVInput();

    const AVCodecContext *videoCodecContext() const { return( mVideoCodecContext ); }
    const AVCodecContext *audioCodecContext() const { return( mAudioCodecContext ); }

    const std::string& source() const { return( mSource ); } 

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
    int decodePacket( AVPacket &packet, int &frameComplete );

    int run();
};

#endif // OZ_AV_INPUT_H


/*@}*/

/** @addtogroup Consumers */
/*@{*/

#ifndef OZ_PARAMETERS_H
#define OZ_PARAMETERS_H

#include "../base/ozFfmpeg.h"

///
/// Video generation/encoding parameters
///
struct VideoParms
{
public:
    uint16_t        mWidth;
    uint16_t        mHeight;
    PixelFormat     mPixelFormat;
    FrameRate       mFrameRate;
    uint32_t        mBitRate;
    uint8_t         mQuality;

public:
    VideoParms( uint16_t width=320, uint16_t height=240, PixelFormat pixelFormat=PIX_FMT_YUV420P, const FrameRate &frameRate=25, uint32_t bitRate=200000, uint8_t quality=70 ) :
        mWidth( width ),
        mHeight( height ),
        mPixelFormat( pixelFormat ),
        mFrameRate( frameRate ),
        mBitRate( bitRate ),
        mQuality( quality )
    {
    }
    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    PixelFormat pixelFormat() const { return( mPixelFormat ); }
    uint16_t bitRate() const { return( mBitRate ); }
    FrameRate frameRate() const { return( mFrameRate ); }
};

///
/// Audio generation/encoding parameters
///
struct AudioParms
{
public:
    AVSampleFormat  mSampleFormat;          ///< The ffmpeg sample format of this frame
    uint32_t        mBitRate;
    uint32_t        mSampleRate;            ///< Sample rate (samples per second) of this frame
    uint8_t         mChannels;              ///< Number of audio channels

public:
    AudioParms( AVSampleFormat sampleFormat=AV_SAMPLE_FMT_S16, uint32_t bitRate=22050, uint32_t sampleRate=16000, uint8_t channels=1 ) :
        mSampleFormat( sampleFormat ),
        mBitRate( bitRate ),
        mSampleRate( sampleRate ),
        mChannels( channels )
    {
    }
    AVSampleFormat sampleFormat() const { return( mSampleFormat ); }
    uint16_t bitRate() const { return( mBitRate ); }
    uint32_t sampleRate() const { return( mSampleRate ); }
    uint8_t channels() const { return( mChannels ); }
};

//class AudioVideoParms : public AudioParms, public VideoParms
//{
    //AudioVideoParms( PixelFormat pixelFormat, uint16_t width=320, uint16_t height=240, const FrameRate &frameRate=FrameRate(1,25), uint32_t bitRate=90000, uint8_t quality=70 ) :
//}

#endif // OZ_PARAMETERS_H
/*@}*/

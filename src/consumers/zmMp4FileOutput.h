#ifndef ZM_MP4_FILE_OUTPUT_H
#define ZM_MP4_FILE_OUTPUT_H

#include "../zmFeedConsumer.h"
#include "../libgen/libgenThread.h"
#include "../zmFfmpeg.h"

///
/// Video generation parameters
///
struct VideoParms
{
public:
    uint16_t        mWidth;
    uint16_t        mHeight;
    FrameRate       mFrameRate;

public:
    VideoParms( uint16_t width=320, uint16_t height=240, const FrameRate &frameRate=FrameRate(1,25) ) :
        mWidth( width ),
        mHeight( height ),
        mFrameRate( frameRate )    {
    }
    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }
};

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

///
/// Consumer that just just writes received video frames to files on the local filesystem.
/// The file will be written to the given location and be called <instance name>-<timestamp>.<format>
///
class Mp4FileOutput : public DataConsumer, public Thread
{
CLASSID(Mp4FileOutput);

protected:
    std::string     mLocation;      ///< Path to directory in which to write movie file
    std::string     mExtension;     ///< File extension for movie file
    uint32_t        mMaxLength;     ///< Maximum length of each movie file, files will be timestamped in name
    VideoParms      mVideoParms;    ///< Video parameters
    AudioParms      mAudioParms;    ///< Audio parameters

protected:
    AVFormatContext *openFile( AVOutputFormat *outputFormat );
    void closeFile( AVFormatContext *outputContext );
    int run();

public:
    Mp4FileOutput( const std::string &name, const std::string &location, uint32_t maxLength, const VideoParms &videoParms, const AudioParms &audioParms ) :
        DataConsumer( cClass(), name, 2 ),
        Thread( identity() ),
        mLocation( location ),
        mExtension( "mp4" ),
        mMaxLength( maxLength ),
        mVideoParms( videoParms ),
        mAudioParms( audioParms )
    {
    }
    ~Mp4FileOutput()
    {
    }
};

#endif // ZM_MP4_FILE_OUTPUT_H

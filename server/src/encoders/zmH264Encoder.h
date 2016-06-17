/** @addtogroup Encoders */
/*@{*/


#ifndef ZM_H264_ENCODER_H
#define ZM_H264_ENCODER_H

//
// Class for H.264 encoder threads.
//

#include "../base/zmEncoder.h"

#include "../libgen/libgenBuffer.h"

///
/// Encoder class that converts received video frames into H.264 video. This is used by the RTSPStream and 
/// RTMPStream classes to produce their network video streams.
///
class H264Encoder : public Encoder, public Thread
{
CLASSID(H264Encoder);

public:
    /* NAL unit types */
    enum {
        NAL_SLICE=1,
        NAL_DPA,
        NAL_DPB,
        NAL_DPC,
        NAL_IDR_SLICE,
        NAL_SEI,
        NAL_SPS,
        NAL_PPS,
        NAL_AUD,
        NAL_END_SEQUENCE,
        NAL_END_STREAM,
        NAL_FILLER_DATA,
        NAL_SPS_EXT,
        NAL_AUXILIARY_SLICE=19
    };

protected:
    uint16_t    mWidth;
    uint16_t    mHeight;
    FrameRate   mFrameRate;
    uint32_t    mBitRate;
    PixelFormat mPixelFormat;
    uint8_t     mQuality;

    int         mAvcLevel;
    int         mAvcProfile;

    ByteBuffer  mSei;
    ByteBuffer  mSps;
    ByteBuffer  mPps;

    ByteBuffer  mInitialFrame;  ///< Contains SEI, SPS and PPS packets

protected:
    static const uint8_t *_findStartCode( const uint8_t *p, const uint8_t *end );

public:
    static const uint8_t *findStartCode( const uint8_t *p, const uint8_t *end );
    static std::string getPoolKey( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality );

public:
    H264Encoder( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality );
    ~H264Encoder();

    const std::string &sdpString( int trackId ) const;

    int gopSize() const;
    int avcLevel() const
    {
        return( mAvcLevel );
    }
    int avcProfile() const
    {
        return( mAvcProfile );
    }
#if 0
    const ByteBuffer sei() const
    {
        return( mLastSei );
    }
    const ByteBuffer sps() const
    {
        return( mLastSps );
    }
    const ByteBuffer pps() const
    {
        return( mLastPps );
    }
#endif

    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }
    uint32_t bitRate() const { return( mBitRate ); }
    PixelFormat pixelFormat() const { return( mPixelFormat ); }
    uint8_t quality() const { return( mQuality ); }

    ///
    /// Register a consumer to this provider. If comparator is null all frames are passed on
    /// otherwise only those for whom the comparator function returns true. Returns true for
    /// success, false for failure.
    ///
    virtual bool registerConsumer( FeedConsumer &consumer, const FeedLink &link=gQueuedFeedLink );

    ///
    /// Deregister a consumer from this provider. If reciprocate is true then also
    /// deregister this provider from the consumer. Returns true for success, false for failure.
    ///
    virtual bool deregisterConsumer( FeedConsumer &consumer, bool reciprocate=true );

protected:
    void poolingExpired() { stop(); }
    int run();
};

#endif // ZM_H264_ENCODER_H


/*@}*/

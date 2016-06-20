/** @addtogroup Encoders */
/*@{*/


#ifndef OZ_MPEG_ENCODER_H
#define OZ_MPEG_ENCODER_H

//
// Class for MPEG encoder threads.
//

#include "../base/ozEncoder.h"

#include "../libgen/libgenBuffer.h"

///
/// Encoder class that converts received video frames into MPEG video. This is used by the RTSPStream and 
/// RTMPStream classes to produce their network video streams.
///
class MpegEncoder : public Encoder, public Thread
{
CLASSID(MpegEncoder);

protected:
    uint16_t        mWidth;
    uint16_t        mHeight;
    FrameRate       mFrameRate;
    uint32_t        mBitRate;
    AVPixelFormat   mPixelFormat;
    uint8_t         mQuality;

    int             mAvcProfile;

public:
    static std::string getPoolKey( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality );

public:
    MpegEncoder( const std::string &name, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality );
    ~MpegEncoder();

    const std::string &sdpString( int trackId ) const;

    int gopSize() const;

    uint16_t width() const { return( mWidth ); }
    uint16_t height() const { return( mHeight ); }
    FrameRate frameRate() const { return( mFrameRate ); }
    uint32_t bitRate() const { return( mBitRate ); }
    AVPixelFormat pixelFormat() const { return( mPixelFormat ); }
    uint8_t quality() const { return( mQuality ); }

    int avcProfile() const
    {
        return( mAvcProfile );
    }

    ///
    /// Register a consumer to this provider. If comparator is null all frames are passed on
    /// otherwise only those for whom the comparator function returns true. Returns true for
    /// success, false for failure.
    ///
    //virtual bool registerConsumer( FeedConsumer &consumer, const FeedLink &link=gQueuedFeedLink );

    ///
    /// Deregister a consumer from this provider. If reciprocate is true then also
    /// deregister this provider from the consumer. Returns true for success, false for failure.
    ///
    //virtual bool deregisterConsumer( FeedConsumer &consumer, bool reciprocate=true );

protected:
    void poolingExpired() { stop(); }
    int run();
};

#endif // OZ_MPEG_ENCODER_H


/*@}*/

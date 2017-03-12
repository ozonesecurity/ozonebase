#ifndef OZ_ENCODER_H
#define OZ_ENCODER_H

#include "ozFeedProvider.h"
#include "ozFeedConsumer.h"

#include <map>

struct AVCodecContext;

///
/// Base class for encoder threads. These are threads that take the captured video
/// frame from a decoder and encode it into another format for transmission. Encoders
/// are normally instantiated 'on-demand' from network interfaces and used to generate
/// video or audio streams. Further requests for the same type of stream will re-use the
/// same encoder. Once no client is using those streams, the encoder will expire itself
/// and be deleted.
///

class Encoder : public VideoConsumer, public VideoProvider
{
private:
    static const int POOL_TIMEOUT = 15;     ///< How long (in seconds) before expiry after the last client has disconnected.

private:
    typedef std::map<std::string,Encoder *>  EncoderMap;

private:
    static Mutex        smPoolMutex;        ///< Protects the static encoder pool from simultaneous access.
    static EncoderMap   smEncoderPool;      ///< Static pool of available encoders.
    
protected:
    AVCodecContext      *mCodecContext;     ///< Ffmpeg codec context pointer, see libavcodec
    bool                mPooled;            ///< Whether the encoder is pooled or not
    std::string         mPoolKey;           ///< Key used for encoder pooling
    time_t              mLastUse;           ///< When the encoder was last used. To enable timed expiry of encoders from pool.
    mutable std::string mSdpString;         ///< For streaming protocol contains the SDP session description for the stream

public:
    static Encoder *getPooledEncoder( const std::string &key ); ///< Request an encoder with the supplied characteristics 
                                                                ///< from the encoder pool.
    static bool poolEncoder( Encoder *encoder );                ///< Add the supplied encoder to the encoder pool
    static bool unpoolEncoder( Encoder *encoder );              ///< Remove the supplied encoder from the encoder pool

public:
    Encoder( const std::string &cl4ss, const std::string &name );
    ~Encoder();

    bool queueFrame( const FramePtr &, FeedProvider * );

    virtual const std::string &sdpString( int trackId ) const ///< Virtual as some encoders may need to generate it first
    {
        return( mSdpString );
    }

protected:
    void cleanup();
    virtual void poolingExpired()=0;

    void distributeFrame( const FramePtr &frame )
    {
        mLastUse = time( NULL );
        FeedProvider::distributeFrame( frame );
    }
};

#endif // OZ_ENCODER_H

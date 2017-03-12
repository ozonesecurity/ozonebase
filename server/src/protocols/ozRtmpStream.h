/** @addtogroup Protocols */
/*@{*/


#ifndef OZ_RTMP_STREAM_H
#define OZ_RTMP_STREAM_H

#include "../base/ozStream.h"
#include "../libgen/libgenThread.h"
#include "../libgen/libgenComms.h"
#include "../libgen/libgenBuffer.h"
#include "../base/ozFfmpeg.h"

class RtmpConnection;
class RtmpSession;
class H264Relay;

// Class representing an RTMP stream.
class RtmpStream : public Stream, public Thread
{
CLASSID(RtmpStream);

private:
    RtmpSession     *mRtmpSession;      // Pointer to the current RTMP session

    RtmpConnection  *mRtmpConnection;

    int             mStreamId;
    //std::string     mAppName;           // Name of stream
    //std::string     mStreamName;        // Name of stream

    H264Relay     *mH264Relay;

    bool            mFirstFrame;
    int             mInitialFrames;

public:
    RtmpStream( RtmpSession *session, RtmpConnection *connection, int streamId, FeedProvider *provider, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality );
    ~RtmpStream();

public:
    //const std::string &name() const
    //{
        //return( mStreamName );
    //}
    bool sendFrame( const FramePtr &frame );
 
protected:
    ByteBuffer *buildPacket( uint32_t chunkStream, uint8_t messageTypeId, uint32_t messageStreamId, uint32_t timestamp, ByteBuffer &payload, int packetType );
    int run();
};

#endif // OZ_RTMP_STREAM_H


/*@}*/

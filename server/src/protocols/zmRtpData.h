#ifndef ZM_RTP_DATA_H
#define ZM_RTP_DATA_H

//
// These are classes that define and control the RTP data streams that contain the H.264 frames
// See http://www.ietf.org/rfc/rfc3550.txt for details of general frame structures etc and
// http://www.ietf.org/rfc/rfc3984.txt for H.264 specific considerations
//

#include "../base/zmStream.h"
#include "../libgen/libgenBuffer.h"
#include "../libgen/libgenThread.h"

extern "C" {
#include <libavutil/rational.h>
}

class RtspStream;
class RtpSession;

// General RTP data packet header
struct RtpDataHeader
{
    uint8_t cc:4;         // CSRC count
    uint8_t x:1;          // header extension flag
    uint8_t p:1;          // padding flag
    uint8_t version:2;    // protocol version
    uint8_t pt:7;         // payload type
    uint8_t m:1;          // marker bit
    uint16_t seqN;        // sequence number, network order
    uint32_t timestampN;  // timestamp, network order
    uint32_t ssrcN;       // synchronization source, network order
    uint32_t csrc[];      // optional CSRC list
};

// Class representing a single RTP data stream. Full audio/video RTP streaming will
// require one of these each for audio and video plus an RTP control stream for each also
class RtpDataManager
{
private:
    RtspStream &mRtspStream;    // Reference to the RTSP streamer
    RtpSession &mRtpSession;    // Reference to the RTP session object

    uint8_t mPayloadType;       // What the contents of the RTP stream are defined as
    AVRational mFrameRate;      // The stream frame rate
    AVRational mClockRate;      // The stream clock rate used to calculate the tick increment between packets

    uint64_t mTimestampBase;    // The initial base time used to offset packets from, currently just 0
    int mSeq;                   // The sequence number of RTP packets, currently starts at 0 but doesn't have to

    PacketQueue mPacketQueue;
    Mutex       mQueueMutex;

public:
    RtpDataManager( RtspStream &rtspStream, RtpSession &rtpSession, uint8_t payloadType, const AVRational &frameRate, const AVRational &clockRate );
    ~RtpDataManager();

    void buildPacket( const unsigned char *data, int size, uint32_t timestamp, bool last );
    PacketQueue &packetQueue() { return( mPacketQueue ); }
    void lockPacketQueue() { mQueueMutex.lock(); }
    void unlockPacketQueue() { mQueueMutex.unlock(); }
};

#endif // ZM_RTP_DATA_H

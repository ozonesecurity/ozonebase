#ifndef ZM_RTMP_SESSION_H
#define ZM_RTMP_SESSION_H

#include "../base/zmConnection.h"

#include <map>

#define ZM_RTMP_CSID_LOW_LEVEL             2
#define ZM_RTMP_CSID_HIGH_LEVEL            3
#define ZM_RTMP_CSID_CONTROL               4
#define ZM_RTMP_CSID_VIDEO                 8
#define ZM_RTMP_CSID_AUDIO                 9

#define ZM_RTMP_TYPE_SET_CHUNK_SIZE        1
#define ZM_RTMP_TYPE_ABORT_MESSAGE         2
#define ZM_RTMP_TYPE_ACK                   3
#define ZM_RTMP_TYPE_USER_CTRL_MESSAGE     4
#define ZM_RTMP_TYPE_WINDOW_ACK_SIZE       5
#define ZM_RTMP_TYPE_SET_PEER_BANDWIDTH    6

#define ZM_RTMP_TYPE_AUDIO                 8
#define ZM_RTMP_TYPE_VIDEO                 9

#define ZM_RTMP_TYPE_DATA_AMF3             15
#define ZM_RTMP_TYPE_SHOBJ_AMF3            16
#define ZM_RTMP_TYPE_CMD_AMF3              17
#define ZM_RTMP_TYPE_DATA_AMF0             18
#define ZM_RTMP_TYPE_SHOBJ_AMF0            19
#define ZM_RTMP_TYPE_CMD_AMF0              20
#define ZM_RTMP_TYPE_AGGREGATE             22

#define RTCP_CMD_SUPPORT_SND_NONE       0x0001
#define RTCP_CMD_SUPPORT_SND_ADPCM      0x0002
#define RTCP_CMD_SUPPORT_SND_MP3        0x0004
#define RTCP_CMD_SUPPORT_SND_UNUSED     0x0008
#define RTCP_CMD_SUPPORT_SND_AAC        0x0400
#define RTCP_CMD_SUPPORT_SND_ALL        0x0fff

#define RTCP_CMD_SUPPORT_VID_NONE       0x0001  // Obsolete
#define RTCP_CMD_SUPPORT_VID_JPEG       0x0002  // Obsolete
#define RTCP_CMD_SUPPORT_VID_H264       0x0080
#define RTCP_CMD_SUPPORT_VID_ALL        0x00ff

#define RTCP_CMD_SUPPORT_VID_CLIENT_SEEK    1

#define ZM_RTMP_CMD_OBJ_ENCODING_AMF0      0
#define ZM_RTMP_CMD_OBJ_ENCODING_AMF3      3

/// General RTMP message header
struct RtmpMessageHeader
{
    uint8_t type;        // Message type
    uint32_t length:24 __attribute__ ((packed));  // Payload length
    uint32_t timestamp __attribute__ ((packed));  // Timestamp
    uint32_t streamId:24 __attribute__ ((packed));// Stream ID
};

struct RtmpSetChunkSizePayload
{
    //RtmpMessageHeader   header;
    uint32_t                 chunkSize __attribute__ ((packed));
};

struct RtmpAbortPayload
{
    //RtmpMessageHeader   header;
    uint32_t                 chunkStreamId __attribute__ ((packed));
};

struct RtmpAckPayload
{
    //RtmpMessageHeader   header;
    uint32_t                 sequenceNumber __attribute__ ((packed));
};

struct RtmpUserControlPayload
{
    //RtmpMessageHeader   header;
    uint16_t            eventType __attribute__ ((packed));
#define ZM_RTMP_UCM_EVENT_STREAM_BEGIN         0
#define ZM_RTMP_UCM_EVENT_STREAM_EOF           1
#define ZM_RTMP_UCM_EVENT_STREAM_DRY           2
#define ZM_RTMP_UCM_EVENT_SET_BUFFER_LENGTH    3
#define ZM_RTMP_UCM_EVENT_STREAM_IS_RECORDED   4
#define ZM_RTMP_UCM_EVENT_PING_REQUEST         6
#define ZM_RTMP_UCM_EVENT_PING_RESPONSE        7
    uint8_t             eventData[];
};

struct RtmpWindowAckSizePayload
{
    //RtmpMessageHeader   header;
    uint32_t             ackWindowSize __attribute__ ((packed));
};

struct RtmpSetPeerBandwidthPayload
{
    //RtmpMessageHeader   header;
    uint32_t             ackWindowSize __attribute__ ((packed));
#define ZM_RTMP_LIMIT_TYPE_HARD    0
#define ZM_RTMP_LIMIT_TYPE_SOFT    1
#define ZM_RTMP_LIMIT_TYPE_DYNAMIC 2
    uint8_t              limitType;
};

class RtmpController;
class RtmpConnection;
class RtmpStream;
class RtmpRequest;

/// Class representing one RTMP session from start to finish.
class RtmpSession
{
private:
    typedef enum { UNINITIALIZED, INITIALIZED, CONNECTED, STREAM_CREATED, PLAYING, STOPPED } State;

    typedef std::map<int,RtmpStream *> StreamMap;
    typedef std::map<int,RtmpRequest *> RequestMap;

private:
    State   mState;

    uint32_t    mSession;                       ///< Session identifier
    std::string mStreamName;
    std::string mStreamSource;
    std::string mClientId;

    RtmpConnection *mConnection;
    StreamMap mRtmpStreams;

    RequestMap mRequests;

public:
    RtmpSession( int session, RtmpConnection *connection );
    ~RtmpSession();

public:
    bool recvRequest( ByteBuffer &request, ByteBuffer &response );

    bool addResponsePayload( ByteBuffer &response, uint32_t chunkStreamId, ByteBuffer &payload );
    bool buildResponse0( ByteBuffer &response, uint32_t chunkStreamId, uint8_t messageTypeId, uint32_t messageStreamId, ByteBuffer &payload );
    bool buildResponse1( ByteBuffer &response, uint32_t chunkStreamId, uint8_t messageTypeId, uint32_t timestampDelta, ByteBuffer &payload );
    bool buildResponse2( ByteBuffer &response, uint32_t chunkStreamId, uint32_t timestampDelta, ByteBuffer &payload );

    uint32_t session() const { return( mSession ); }

protected:
    bool handleRequest( uint32_t chunkStreamId, RtmpRequest &request, ByteBuffer &response );
};

#endif // ZM_RTMP_SESSION_H

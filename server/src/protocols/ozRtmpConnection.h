/** @addtogroup Protocols */
/*@{*/


#ifndef ZM_RTMP_CONNECTION_H
#define ZM_RTMP_CONNECTION_H

#include "../base/ozConnection.h"
#include "ozRtmpRequest.h"

#include "../libgen/libgenThread.h"

class RtmpController;

// Class representing a single TCP connection to the RTMP server. This does not correlate with a specific
// session but is used primarily to maintain state and content between requests, especially partial ones,
// and manage access to the socket.
class RtmpConnection : public BinaryConnection
{
private:
    typedef enum { UNITIALIZED, GOT_C0, SENT_S2, HANDSHAKE_DONE } State;

public:
    static const uint8_t CS0 = 3;

    struct CS1
    {
        uint32_t time;           // Base time of Epoch, probably zero
        uint32_t zero;           // Zero bytes
        uint8_t random[1528];    // Random bytes
    };

    struct CS2
    {
        uint32_t time;           // Base time of Epoch, probably zero
        uint32_t time2;          // Zero bytes
        uint8_t random[1528];    // Random bytes
    };

    struct __attribute__ ((__packed__)) ChunkHeader0
    {
        uint32_t timestamp:24;
        uint32_t messageLength:24;
        uint8_t  messageTypeId;
        uint32_t messageStreamId;
    };

    struct __attribute__ ((__packed__)) ChunkHeader1
    {
        uint32_t timestampDelta:24;
        uint32_t messageLength:24;
        uint8_t  messageTypeId;
    };

    struct __attribute__ ((__packed__)) ChunkHeader2
    {
        uint32_t timestampDelta:24;
    };

private:
    static int  smConnectionId;

private:
    RtmpController  *mRtmpController;

    int             mTxChunkSize;
    int             mRxChunkSize;
    int             mWindowAckSize;
    uint32_t        mBaseTimestamp;
    int             mServerDigestPosition;

    int             mConnectionId;

    State           mState;

    bool            mFP9;

    CS1             mC1;
    CS1             mS1;
    int             mC1size;
    CS2             mC2;
    CS2             mS2;
    int             mC2size;

    Mutex           mSocketMutex;

public:
    RtmpConnection( Controller *controller, TcpInetSocket *socket );
    ~RtmpConnection();

public:
    uintptr_t sessionId()
    {
        return( reinterpret_cast<uintptr_t>( this ) );
    }

    uint16_t txChunkSize() const
    {
        return( mTxChunkSize );
    }
    void txChunkSize( uint16_t chunkSize )
    {
        mTxChunkSize = chunkSize;
    }
    uint16_t rxChunkSize() const
    {
        return( mRxChunkSize );
    }
    void rxChunkSize( uint16_t chunkSize )
    {
        mRxChunkSize = chunkSize;
    }

public:
    uint32_t rtmpTimestamp( int64_t time=0 );
    uint32_t rtmpTimestamp24();
    uint32_t rtmpTimestamp32();

    bool recvRequest( ByteBuffer &request );
    bool sendResponse( int chunkStreamId, uint8_t messageTypeId, uint32_t messageStreamId, void *payloadData, size_t payloadSize ); // CH0
    bool sendResponse( int chunkStreamId, uint8_t messageTypeId, uint32_t messageStreamId, ByteBuffer &payload ); // CH0
    bool sendResponse( int chunkStreamId, uint8_t messageTypeId, ByteBuffer &payload, uint32_t timestampDelta ); // CH1
    bool sendResponse( int chunkStreamId, ByteBuffer &payload, uint32_t timestampDelta ); // CH2
    bool sendResponse( const void *messageData, size_t messageSize );
};

#endif // ZM_RTMP_CONNECTION_H


/*@}*/

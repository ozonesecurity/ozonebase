#ifndef ZM_RTMP_REQUEST_H
#define ZM_RTMP_REQUEST_H

#include "../libgen/libgenBuffer.h"

#include <stdint.h>

class RtmpRequest
{
private:
    uint8_t      mMessageType;
    uint32_t     mMessageStreamId;
    uint32_t     mTimestamp;
    uint32_t     mMessageLength;

    ByteBuffer  mPayload;

public:
    RtmpRequest() :
        mMessageType( 0 ),
        mMessageStreamId( 0 ),
        mTimestamp( 0 ),
        mMessageLength( 0 )
    {
    }
    RtmpRequest( uint8_t messageType, uint32_t messageStreamId, uint32_t timestamp, uint32_t requestLength ) :
        mMessageType( messageType ),
        mMessageStreamId( messageStreamId ),
        mTimestamp( timestamp ),
        mMessageLength( requestLength )
    {
        mPayload.reserve( requestLength );
    }
    RtmpRequest( uint8_t messageType, uint32_t messageStreamId, uint32_t timestamp, uint8_t *requestData, uint32_t requestLength ) :
        mMessageType( messageType ),
        mMessageStreamId( messageStreamId ),
        mTimestamp( timestamp ),
        mMessageLength( requestLength ),
        mPayload( requestData, requestLength )
    {
    }
    ~RtmpRequest()
    {
    }

    uint8_t messageType() const
    {
        return( mMessageType );
    }
    void messageType( uint8_t newMessageType )
    {
        mMessageType = newMessageType;
    }

    uint32_t messageStreamId() const
    {
        return( mMessageStreamId );
    }
    void messageStreamId( uint32_t newMessageStreamId )
    {
        mMessageStreamId = newMessageStreamId;
    }

    uint32_t messageLength() const
    {
        return( mMessageLength );
        //return( mPayload.size() );
    }
    void messageLength( uint32_t newMessageLength )
    {
        mMessageLength = newMessageLength;
        mPayload.reserve( newMessageLength );
    }

    uint32_t timestamp() const
    {
        return( mTimestamp );
    }
    void timestamp( uint32_t timestamp )
    {
        mTimestamp = timestamp;
    }
    void timestampDelta( uint32_t delta )
    {
        mTimestamp += delta;
    }

    const ByteBuffer &payload() const
    {
        return( mPayload );
    }
    ByteBuffer &payload()
    {
        return( mPayload );
    }

    bool complete() const
    {
        return( mPayload.size() == mMessageLength );
    }

    bool addPayload( uint8_t *data, uint32_t length );
    void clearPayload();

    bool build( int format, ByteBuffer &request );
};

#endif // ZM_RTMP_REQUEST_H

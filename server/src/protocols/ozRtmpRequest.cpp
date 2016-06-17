#include "../base/oz.h"
#include "ozRtmpRequest.h"

#include "ozRtmpConnection.h"

bool RtmpRequest::addPayload( uint8_t *data, uint32_t length )
{
    mPayload.append( data, length );
    if ( mPayload.size() > mMessageLength )
    {
        Panic( "Message payload size %zd greater than expected length %d", mPayload.size(), mMessageLength );
    }
    return( true );
}

void RtmpRequest::clearPayload()
{
    mPayload.clear();
}

bool RtmpRequest::build( int format, ByteBuffer &payload )
{
    switch( format )
    {
        case 0:
        {
            RtmpConnection::ChunkHeader0 header;
            memcpy( &header, payload.data(), sizeof(header) );
            Debug( 2, "CH0 - timestamp: %d, messageLength: %d, messageTypeId: %d, messageStreamId: %d",
                be32toh(header.timestamp<<8), be32toh(header.messageLength<<8), header.messageTypeId, le32toh(header.messageStreamId) );
            payload -= sizeof(header);

            mMessageType = header.messageTypeId;
            mMessageStreamId = le32toh(header.messageStreamId);
            mTimestamp = be32toh(header.timestamp<<8);
            mMessageLength = be32toh(header.messageLength<<8);
            break;
        }
        case 1:
        {
            //if ( !mTimestamp )
                //Fatal( "No existing RTMP timestamp found in request header for chunk stream Type 1 request" ); 

            RtmpConnection::ChunkHeader1 header;
            memcpy( &header, payload.data(), sizeof(header) );
            Debug( 2, "CH1 - timestampDelta: %d, messageLength: %d, messageTypeId: %d",
                be32toh(header.timestampDelta<<8), be32toh(header.messageLength<<8), header.messageTypeId );
            payload -= sizeof(header);

            mMessageType = header.messageTypeId;
            mTimestamp += be32toh(header.timestampDelta<<8);
            mMessageLength = be32toh(header.messageLength<<8);
            break;
        }
        case 2:
        {
            //if ( !mTimestamp )
                //Fatal( "No existing RTMP timestamp found in request header for chunk stream Type 2 request" ); 

            RtmpConnection::ChunkHeader2 header;
            memcpy( &header, payload.data(), sizeof(header) );
            Debug( 2, "CH2 - timestampDelta: %d", be32toh(header.timestampDelta<<8) );
            payload -= sizeof(header);

            mTimestamp += be32toh(header.timestampDelta<<8);
            break;
        }
        case 3: // No header
        {
            // Do nothing
            break;
        }
        default:
        {
            Fatal( "Unexpected RTMP request header format %d", format ); 
            break;
        }
    }
    return( true );
}

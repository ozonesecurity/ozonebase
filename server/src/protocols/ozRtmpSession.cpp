#include "../base/oz.h"
#include "ozRtmpSession.h"

#include "ozRtmp.h"
#include "ozRtmpConnection.h"
#include "ozRtmpStream.h"
#include "ozRtmpAmf.h"
#include "../encoders/ozH264Encoder.h"
#include "../libgen/libgenUtils.h"

RtmpSession::RtmpSession( int session, RtmpConnection *connection ) :
    mState( UNINITIALIZED ),
    mSession( session ),
    mConnection( connection )
{
    Debug( 1, "New RTMP session %x", mSession );
    while( mClientId.length() < 8 )
    {
        char clientChar = 0;
        while( !clientChar || !isalnum(clientChar) )
            clientChar = rand()%128;
        mClientId += clientChar;
    }
    Debug( 2, "Client ID: %s", mClientId.c_str() );
}

RtmpSession::~RtmpSession()
{
    Debug( 1, "Closing RTMP session" );
    for ( StreamMap::iterator iter = mRtmpStreams.begin(); iter != mRtmpStreams.end(); iter++ )
    {
        RtmpStream *rtmpStream = iter->second;
        rtmpStream->stop();
        rtmpStream->join();
        delete rtmpStream;
    }
    mRtmpStreams.clear();
}

bool RtmpSession::addResponsePayload( ByteBuffer &response, uint32_t chunkStreamId, ByteBuffer &payload )
{
    while ( payload.size() )
    {
        size_t chunkSize = min(mConnection->txChunkSize(),payload.size());
        response.append( payload.data(), chunkSize );

        payload -= chunkSize;

        if ( payload.size() )
            response.append( 0xc0 | chunkStreamId );
    }
    return( true );
}

// Header type 0
bool RtmpSession::buildResponse0( ByteBuffer &response, uint32_t chunkStreamId, uint8_t messageTypeId, uint32_t messageStreamId, ByteBuffer &payload )
{
    response.append( chunkStreamId&0x3f );

    RtmpConnection::ChunkHeader0 header;
    header.timestamp = mConnection->rtmpTimestamp24();
    header.messageLength = htobe24(payload.size());
    header.messageTypeId = messageTypeId;
    header.messageStreamId = htole32(messageStreamId);
    response.append( &header, sizeof(header) );

    addResponsePayload( response, chunkStreamId, payload );

    return( true );
}

// Header type 1
bool RtmpSession::buildResponse1( ByteBuffer &response, uint32_t chunkStreamId, uint8_t messageTypeId, uint32_t timestampDelta, ByteBuffer &payload )
{
    response.append( 0x40|(chunkStreamId&0x3f) );

    RtmpConnection::ChunkHeader1 header;
    header.timestampDelta = htobe24(timestampDelta);
    header.messageLength = htobe24(payload.size());
    header.messageTypeId = messageTypeId;
    response.append( &header, sizeof(header) );

    addResponsePayload( response, chunkStreamId, payload );

    return( true );
}

// Header type 2
bool RtmpSession::buildResponse2( ByteBuffer &response, uint32_t chunkStreamId, uint32_t timestampDelta, ByteBuffer &payload )
{
    response.append( 0x80|(chunkStreamId&0x3f) );

    RtmpConnection::ChunkHeader2 header;
    header.timestampDelta = htobe24(timestampDelta);
    response.append( &header, sizeof(header) );

    addResponsePayload( response, chunkStreamId, payload );

    return( true );
}

bool RtmpSession::handleRequest( uint32_t chunkStreamId, RtmpRequest &request, ByteBuffer &response )
{
    Debug( 2, "Handling %zd byte request type %d, stream %d", request.payload().size(), request.messageType(), request.messageStreamId() );
    //
    //uint8_t amfVersion = request--;
    switch( mState )
    {
        case UNINITIALIZED :
        {
            switch( request.messageType() )
            {
                case OZ_RTMP_TYPE_WINDOW_ACK_SIZE :
                {
                    RtmpWindowAckSizePayload *payload = reinterpret_cast<RtmpWindowAckSizePayload *>( request.payload().data() );
                    Debug( 2, "Got AckWindowSize %d", be32toh(payload->ackWindowSize) );
                    // Send stream begin
                    {
                        RtmpUserControlPayload ucPayload;
                        ucPayload.eventType = htobe16(OZ_RTMP_UCM_EVENT_STREAM_BEGIN);
                        uint32_t eventData = 0;

                        ByteBuffer payload;
                        payload.append( &ucPayload, sizeof(ucPayload) );
                        payload.append( &eventData, sizeof(eventData) );

                        buildResponse0( response, OZ_RTMP_CSID_LOW_LEVEL, OZ_RTMP_TYPE_USER_CTRL_MESSAGE, 0, payload );
                        Debug( 2, "Built User Control Message - Stream Begin" );
                    }
                    request.clearPayload();
                    return( true );
                    break;
                }
                case OZ_RTMP_TYPE_USER_CTRL_MESSAGE :
                {
                    RtmpUserControlPayload *payload = reinterpret_cast<RtmpUserControlPayload *>( request.payload().data() );
                    Debug( 2, "Got UserControl event %d", be16toh(payload->eventType) );
                    request.clearPayload();
                    return( true );
                    break;
                }
                case OZ_RTMP_TYPE_CMD_AMF3 :
                {
                    Debug( 2, "Removing first byte %02x", request.payload()[0] );
                    request.payload()--;
                    // Fallthru
                }
                case OZ_RTMP_TYPE_CMD_AMF0 :
                {
                    uint8_t amfVersion = 0;
                    if ( amfVersion == 0 )
                    {
                        Hexdump( 2, request.payload().data(), request.payload().size() );
                        typedef std::vector<Amf0Record *> RecordList;
                        RecordList records;
                        while ( request.payload().size() )
                        {
                            records.push_back( new Amf0Record( request.payload() ) );
                        }
                        Debug( 2, "Got %zd records", records.size() );
                        if ( records.size() > 1 )
                        {
                            std::string command = records[0]->getString();
                            int transactionId = records[1]->getInt();
                            const Amf0Object &parameters = records[2]->getObject();
                            int objectEncoding = OZ_RTMP_TYPE_CMD_AMF3;
                            const Amf0Record *appRecord = parameters.getRecord( "app" );
                            if ( appRecord )
                            {
                                mStreamName = appRecord->getString();
                                Debug( 2, "Got app '%s'", mStreamName.c_str() );
                            }
                            else
                            {
                                Error( "Can't find app parameter" );
                            }
                            const Amf0Record *objectEncodingRecord = parameters.getRecord( "objectEncoding" );
                            if ( objectEncodingRecord )
                                objectEncoding = objectEncodingRecord->getInt();
                            else
                                Warning( "Can't find object encoding, defaulting to %d", objectEncoding );
                            Debug( 2, "Got command '%s', transaction id %d", command.c_str(), transactionId );
                            if ( command == "connect" )
                            {
                                // Send Window Ack Size message
                                {
                                    ByteBuffer payload;
                                    RtmpWindowAckSizePayload messagePayload;
                                    //messagePayload.ackWindowSize = htobe32(65536); //????
                                    messagePayload.ackWindowSize = htobe32(2500000); //????
                                    payload.adopt( reinterpret_cast<unsigned char *>(&messagePayload), sizeof(messagePayload) );

                                    buildResponse0( response, OZ_RTMP_CSID_LOW_LEVEL, OZ_RTMP_TYPE_WINDOW_ACK_SIZE, 0, payload );
                                    Debug( 2, "Built Window Ack Size Message" );
                                }
                                // Send Set Peer Bandwidth message
                                {
                                    ByteBuffer payload;
                                    RtmpSetPeerBandwidthPayload messagePayload;
                                    //messagePayload.ackWindowSize = htobe32(65536); //????
                                    messagePayload.ackWindowSize = htobe32(2500000); //????
                                    messagePayload.limitType = OZ_RTMP_LIMIT_TYPE_DYNAMIC;
                                    payload.adopt( reinterpret_cast<unsigned char *>(&messagePayload), sizeof(messagePayload) );

                                    buildResponse0( response, OZ_RTMP_CSID_LOW_LEVEL, OZ_RTMP_TYPE_SET_PEER_BANDWIDTH, 0, payload );
                                    Debug( 2, "Built Set Peer Bandwidth Message" );
                                }

                                {
                                    RtmpUserControlPayload ucPayload;
                                    ucPayload.eventType = htobe16(OZ_RTMP_UCM_EVENT_STREAM_BEGIN);
                                    uint32_t eventData = 0;

                                    ByteBuffer payload;
                                    payload.append( &ucPayload, sizeof(ucPayload) );
                                    payload.append( &eventData, sizeof(eventData) );

                                    buildResponse0( response, OZ_RTMP_CSID_LOW_LEVEL, OZ_RTMP_TYPE_USER_CTRL_MESSAGE, 0, payload );
                                    Debug( 2, "Built User Control Message - Stream Begin" );
                                }

                                // Send _result
                                {
                                    Amf0Record amfCommand( "_result" );
                                    Amf0Record amfTransactionId( 1 );
                                    Amf0Object properties;
                                    properties.addRecord( "fmsVer", Amf0Record( "FMS/3,5,1,516" ) );
                                    properties.addRecord( "capabilities", Amf0Record( 31 ) );
                                    properties.addRecord( "mode", Amf0Record( 1 ) );
                                    Amf0Object information;
                                    information.addRecord( "level", "status" );
                                    if ( mConnection->controller()->verifyStreamName( mStreamName ) )
                                    {
                                        information.addRecord( "code", "NetConnection.Connect.Success" );
                                        information.addRecord( "description", "Connection Succeeded." );
                                    }
                                    else
                                    {
                                        Error( "Invalid app record received '%s'", mStreamName.c_str() );
                                        information.addRecord( "code", "NetConnection.Connect.Failed" );
                                        information.addRecord( "description", "Connection Failed." );
                                    }
                                    //information.addRecord( "objectEncoding", OZ_RTMP_CMD_OBJ_ENCODING_AMF3 );
                                    information.addRecord( "objectEncoding", objectEncoding );
                                    Amf0Array version;
                                    version.addRecord( "version", "3,5,1,516" );
                                    information.addRecord( "data", version );

                                    ByteBuffer payload;
                                    amfCommand.encode( payload );
                                    amfTransactionId.encode( payload );
                                    properties.encode( payload );
                                    information.encode( payload );

                                    buildResponse0( response, OZ_RTMP_CSID_HIGH_LEVEL, OZ_RTMP_TYPE_CMD_AMF0, 0, payload );
                                    Debug( 2, "Built Command Message - _result" );
                                }
                                mState = CONNECTED;
                            }
                        }
                        request.clearPayload();
                        return( true );
                    }
                    else if ( amfVersion == 3 )
                    {
                        Error( "Unrecognised AMF version %d", amfVersion );
                    }
                    else
                    {
                        Error( "Unrecognised AMF version %d", amfVersion );
                    }
                    break;
                }
                default :
                {
                    Error( "Unexpected message type %d received in state %d", request.messageType(), mState );
                }
            }
            break;
        }
        case INITIALIZED :
        {
            switch( request.messageType() )
            {
                case OZ_RTMP_TYPE_WINDOW_ACK_SIZE :
                {
                    RtmpWindowAckSizePayload *payload = reinterpret_cast<RtmpWindowAckSizePayload *>( request.payload().data() );
                    Debug( 2, "Got AckWindowSize %d", le32toh(payload->ackWindowSize) );
                    // Send stream begin
                    {
                        RtmpUserControlPayload ucPayload;
                        ucPayload.eventType = htobe16(OZ_RTMP_UCM_EVENT_STREAM_BEGIN);
                        uint32_t eventData = 0;

                        ByteBuffer payload;
                        payload.append( &ucPayload, sizeof(ucPayload) );
                        payload.append( &eventData, sizeof(eventData) );

                        buildResponse0( response, OZ_RTMP_CSID_LOW_LEVEL, OZ_RTMP_TYPE_USER_CTRL_MESSAGE, 0, payload );
                        Debug( 2, "Built User Control Message - Stream Begin" );
                    }
                    // Send _result
                    {
                        Amf0Record amfCommand( "_result" );
                        Amf0Record amfTransactionId( 1 );
                        Amf0Object properties;
                        properties.addRecord( "fmsVer", Amf0Record( "FMS/3,5,1,516" ) );
                        properties.addRecord( "capabilities", Amf0Record( 31 ) );
                        properties.addRecord( "mode", Amf0Record( 1 ) );
                        Amf0Object information;
                        information.addRecord( "level", "status" );
                        information.addRecord( "code", "NetConnection.Connect.Success" );
                        information.addRecord( "description", "Connection Succeeded." );
                        information.addRecord( "objectEncoding", OZ_RTMP_CMD_OBJ_ENCODING_AMF3 );
                        Amf0Object version;
                        version.addRecord( "version", "3,5,1,516" );
                        information.addRecord( "data", version );

                        ByteBuffer payload;
                        amfCommand.encode( payload );
                        amfTransactionId.encode( payload );
                        properties.encode( payload );
                        information.encode( payload );

                        buildResponse0( response, OZ_RTMP_CSID_HIGH_LEVEL, OZ_RTMP_TYPE_CMD_AMF0, 0, payload );
                        Debug( 2, "Built Command Message - _result" );
                    }
                    mState = CONNECTED;
                    request.clearPayload();
                    return( true );
                    break;
                }
                default :
                {
                    Error( "Unexpected message type %d received in state %d", request.messageType(), mState );
                }
            }
            break;
        }
        case CONNECTED :
        {
            switch( request.messageType() )
            {
                case OZ_RTMP_TYPE_WINDOW_ACK_SIZE :
                {
                    RtmpWindowAckSizePayload *payload = reinterpret_cast<RtmpWindowAckSizePayload *>( request.payload().data() );
                    Debug( 2, "Got AckWindowSize %d", be32toh(payload->ackWindowSize) );
                    request.clearPayload();
                    return( true );
                    break;
                }
                case OZ_RTMP_TYPE_USER_CTRL_MESSAGE :
                {
                    RtmpUserControlPayload *payload = reinterpret_cast<RtmpUserControlPayload *>( request.payload().data() );
                    Debug( 2, "Got UserControl event %d", be16toh(payload->eventType) );
                    request.clearPayload();
                    return( true );
                    break;
                }
                case OZ_RTMP_TYPE_CMD_AMF3 :
                {
                    Debug( 2, "Removing first byte %02x", request.payload()[0] );
                    request.payload()--;
                    // Fallthru
                }
                case OZ_RTMP_TYPE_CMD_AMF0 :
                {
                    uint8_t amfVersion = 0;
                    if ( amfVersion == 0 )
                    {
                        Hexdump( 2, request.payload().data(), request.payload().size() );
                        typedef std::vector<Amf0Record *> RecordList;
                        RecordList records;
                        while ( request.payload().size() )
                        {
                            records.push_back( new Amf0Record( request.payload() ) );
                        }
                        Debug( 2, "Got %zd records", records.size() );
                        if ( records.size() > 1 )
                        {
                            std::string command = records[0]->getString();
                            int transactionId = records[1]->getInt();
                            Debug( 2, "Got command '%s', transaction id %d", command.c_str(), transactionId );
                            if ( command == "createStream" )
                            {
                                Amf0Record amfCommand( "_result" );
                                Amf0Record amfTransactionId( transactionId );
                                Amf0Record null;
                                Amf0Record streamId( 1 );

                                ByteBuffer payload;
                                amfCommand.encode( payload );
                                amfTransactionId.encode( payload );
                                null.encode( payload );
                                streamId.encode( payload );

                                buildResponse0( response, chunkStreamId, OZ_RTMP_TYPE_CMD_AMF0, request.messageStreamId(), payload );
                                Debug( 2, "Built Command Message - _result" );

                                mState = STREAM_CREATED;
                            }
                        }
                        request.clearPayload();
                        return( true );
                    }
                    else if ( amfVersion == 3 )
                    {
                        Error( "Unrecognised AMF version %d", amfVersion );
                    }
                    else
                    {
                        Error( "Unrecognised AMF version %d", amfVersion );
                    }
                    break;
                }
                default :
                {
                    Error( "Unexpected message type %d received in state %d", request.messageType(), mState );
                }
            }
            break;
        }
        case STREAM_CREATED :
        case PLAYING :
        {
            switch( request.messageType() )
            {
                case OZ_RTMP_TYPE_USER_CTRL_MESSAGE :
                {
                    RtmpUserControlPayload *payload = reinterpret_cast<RtmpUserControlPayload *>( request.payload().data() );
                    uint16_t eventType = be16toh(payload->eventType);
                    Debug( 2, "Got UserControl event %d", eventType );
                    switch( eventType )
                    {
                        case OZ_RTMP_UCM_EVENT_SET_BUFFER_LENGTH :
                        {
                            uint32_t streamId = be32toh( *(reinterpret_cast<uint32_t *>(&(payload->eventData[0]))) );
                            uint32_t bufferLength = be32toh( *(reinterpret_cast<uint32_t *>(&(payload->eventData[4]))) );
                            Debug( 2, "Setting buffer length for stream %d to %d", streamId, bufferLength );
                            break;
                        }
                        case OZ_RTMP_UCM_EVENT_STREAM_BEGIN :
                        case OZ_RTMP_UCM_EVENT_STREAM_EOF :
                        case OZ_RTMP_UCM_EVENT_STREAM_DRY :
                        case OZ_RTMP_UCM_EVENT_STREAM_IS_RECORDED :
                        case OZ_RTMP_UCM_EVENT_PING_REQUEST :
                        case OZ_RTMP_UCM_EVENT_PING_RESPONSE :
                        default :
                        {
                            Warning( "Unexpected User Control Message event type %d received", eventType );
                        }
                    }
                    request.clearPayload();
                    return( true );
                    break;
                }
                case OZ_RTMP_TYPE_CMD_AMF3 :
                {
                    Debug( 2, "Removing first byte %02x", request.payload()[0] );
                    request.payload()--;
                    // Fallthru
                }
                case OZ_RTMP_TYPE_CMD_AMF0 :
                {
                    uint8_t amfVersion = 0;
                    if ( amfVersion == 0 )
                    {
                        Hexdump( 2, request.payload().data(), request.payload().size() );
                        typedef std::vector<Amf0Record *> RecordList;
                        RecordList records;
                        while ( request.payload().size() )
                        {
                            records.push_back( new Amf0Record( request.payload() ) );
                        }
                        Debug( 2, "Got %zd records", records.size() );
                        if ( records.size() > 1 )
                        {
                            std::string command = records[0]->getString();
                            int transactionId = records[1]->getInt();
                            Debug( 2, "Got command '%s', transaction id %d", command.c_str(), transactionId );
                            if ( command == "play" )
                            {
                                mStreamSource = records[3]->getString();
                                Debug( 2, "Got stream name %s", mStreamSource.c_str() );

                                ByteBuffer payload;
                                {
                                    RtmpSetChunkSizePayload messagePayload;
                                    messagePayload.chunkSize = htobe32( 32768 ); //????
                                    payload.adopt( reinterpret_cast<unsigned char *>(&messagePayload), sizeof(messagePayload) );

                                    buildResponse0( response, OZ_RTMP_CSID_LOW_LEVEL, OZ_RTMP_TYPE_SET_CHUNK_SIZE, 0, payload );
                                    Debug( 2, "Built Set Chunk Size Message" );
                                    mConnection->txChunkSize( 32768 );
                                }
                                // Send stream begin
                                {
                                    RtmpUserControlPayload ucPayload;
                                    ucPayload.eventType = htobe16(OZ_RTMP_UCM_EVENT_STREAM_BEGIN);
                                    uint32_t eventData = 0;

                                    ByteBuffer payload;
                                    payload.append( &ucPayload, sizeof(ucPayload) );
                                    payload.append( &eventData, sizeof(eventData) );

                                    buildResponse0( response, OZ_RTMP_CSID_LOW_LEVEL, OZ_RTMP_TYPE_USER_CTRL_MESSAGE, 1, payload );
                                    Debug( 2, "Built User Control Message - Stream Begin" );
                                }
                                if ( true )
                                {
                                    Amf0Record amfCommand( "onStatus" );
                                    Amf0Record amfTransactionId( transactionId );
                                    Amf0Record properties;
                                    Amf0Object information;
                                    information.addRecord( "level", "status" );
                                    information.addRecord( "code", "NetStream.Play.Reset" );
                                    information.addRecord( "description", stringtf( "Playing and resetting %s.", mStreamSource.c_str() ) );
                                    information.addRecord( "details", mStreamSource );
                                    information.addRecord( "clientid", mClientId );

                                    ByteBuffer payload;
                                    amfCommand.encode( payload );
                                    amfTransactionId.encode( payload );
                                    properties.encode( payload );
                                    information.encode( payload );

                                    buildResponse0( response, OZ_RTMP_CSID_CONTROL, OZ_RTMP_TYPE_CMD_AMF0, request.messageStreamId(), payload );
                                    Debug( 2, "Built Command Message - onStatus" );
                                }
                                FeedProvider *streamProvider = mConnection->controller()->findStream( mStreamName, mStreamSource );
                                if ( streamProvider )
                                {
                                    Amf0Record amfCommand( "onStatus" );
                                    Amf0Record amfTransactionId( transactionId );
                                    Amf0Record properties;
                                    Amf0Object information;
                                    information.addRecord( "level", "status" );
                                    information.addRecord( "code", "NetStream.Play.Start" );
                                    information.addRecord( "description", stringtf( "Started playing %s.", mStreamSource.c_str() ) );
                                    information.addRecord( "details", mStreamSource );
                                    information.addRecord( "clientid", mClientId );

                                    ByteBuffer payload;
                                    amfCommand.encode( payload );
                                    amfTransactionId.encode( payload );
                                    properties.encode( payload );
                                    information.encode( payload );

                                    buildResponse0( response, OZ_RTMP_CSID_CONTROL, OZ_RTMP_TYPE_CMD_AMF0, request.messageStreamId(), payload );
                                    Debug( 2, "Built Command Message - onStatus (NetStream.Play.Start)" );

                                    RtmpStream *rtmpStream = new RtmpStream( this, mConnection, request.messageStreamId(), streamProvider, 320, 240, 10, 90000, 70 );
                                    mRtmpStreams.insert( StreamMap::value_type( request.messageStreamId(), rtmpStream ));
                                    rtmpStream->start();
                                    mState = PLAYING;
                                }
                                else
                                {
                                    {
                                        Amf0Record amfCommand( "onStatus" );
                                        Amf0Record amfTransactionId( transactionId );
                                        Amf0Record properties;
                                        Amf0Object information;
                                        information.addRecord( "level", "status" );
                                        information.addRecord( "code", "NetStream.Play.Failed" );
                                        information.addRecord( "description", stringtf( "Unknown stream %s.", mStreamSource.c_str() ) );
                                        information.addRecord( "details", mStreamSource );
                                        information.addRecord( "clientid", mClientId );

                                        ByteBuffer payload;
                                        amfCommand.encode( payload );
                                        amfTransactionId.encode( payload );
                                        properties.encode( payload );
                                        information.encode( payload );

                                        buildResponse0( response, OZ_RTMP_CSID_CONTROL, OZ_RTMP_TYPE_CMD_AMF0, request.messageStreamId(), payload );
                                        Debug( 2, "Built Command Message - onStatus (NetStream.Play.Failed)" );
                                    }
                                    // Send _result
                                    {
                                        Amf0Record amfCommand( "_result" );
                                        Amf0Record amfTransactionId( 1 );
                                        Amf0Object properties;
                                        properties.addRecord( "fmsVer", Amf0Record( "FMS/3,5,1,516" ) );
                                        properties.addRecord( "capabilities", Amf0Record( 31 ) );
                                        properties.addRecord( "mode", Amf0Record( 1 ) );
                                        Amf0Object information;
                                        information.addRecord( "level", "status" );
                                        information.addRecord( "code", "NetConnection.Connect.Closed" );
                                        information.addRecord( "description", "Connection Closed." );
                                        information.addRecord( "objectEncoding", OZ_RTMP_CMD_OBJ_ENCODING_AMF3 );
                                        Amf0Array version;
                                        version.addRecord( "version", "3,5,1,516" );
                                        information.addRecord( "data", version );

                                        ByteBuffer payload;
                                        amfCommand.encode( payload );
                                        amfTransactionId.encode( payload );
                                        properties.encode( payload );
                                        information.encode( payload );

                                        buildResponse0( response, OZ_RTMP_CSID_HIGH_LEVEL, OZ_RTMP_TYPE_CMD_AMF0, 0, payload );
                                        Debug( 2, "Built Command Message - _result" );
                                    }
                                }
                            }
                            else if ( command == "closeStream" )
                            {
                                for ( StreamMap::iterator iter = mRtmpStreams.begin(); iter != mRtmpStreams.end(); iter++ )
                                {
                                    RtmpStream *rtmpStream = iter->second;

                                    Amf0Record amfCommand( "onStatus" );
                                    Amf0Record amfTransactionId( transactionId );
                                    Amf0Record properties;
                                    Amf0Object information;
                                    information.addRecord( "level", "status" );
                                    information.addRecord( "code", "NetStream.Play.Stop" );
                                    information.addRecord( "description", stringtf( "Stopped playing %s.", mStreamSource.c_str() ) );
                                    information.addRecord( "details", mStreamSource.c_str() );
                                    information.addRecord( "clientid", mClientId );
                                    information.addRecord( "reason", "" );

                                    ByteBuffer payload;
                                    amfCommand.encode( payload );
                                    amfTransactionId.encode( payload );
                                    properties.encode( payload );
                                    information.encode( payload );

                                    buildResponse0( response, OZ_RTMP_CSID_CONTROL, OZ_RTMP_TYPE_CMD_AMF0, request.messageStreamId(), payload );
                                    Debug( 2, "Built Command Message - onStatus (NetStream.Play.Stop)" );

                                    rtmpStream->stop();
                                }
                                mState = STOPPED;
                            }
                        }
                        request.clearPayload();
                        return( true );
                    }
                    else if ( amfVersion == 3 )
                    {
                        Error( "Unrecognised AMF version %d", amfVersion );
                    }
                    else
                    {
                        Error( "Unrecognised AMF version %d", amfVersion );
                    }
                    break;
                }
                default :
                {
                    Error( "Unexpected message type %d received in state %d", request.messageType(), mState );
                }
            }
            break;
        }
        default :
        {
            Error( "Unexpected state %d", mState );
        }
    }
    return( false );
}

bool RtmpSession::recvRequest( ByteBuffer &request, ByteBuffer &response )
{
    while ( request.size() )
    {
        uint8_t format = request[0]>>6;
        int chunkStreamId = request[0]&0x3f;
        request.consume( 1 );
        switch ( chunkStreamId )
        {
            case 0 :
            {
                chunkStreamId = request[0]+64;
                request.consume( 1 );
                break;
            }
            case 1 :
            {
                chunkStreamId = request[1];
                chunkStreamId <<= 8;
                chunkStreamId |= request[0]+64;
                request -= 2;
                break;
            }
            case 2 :
            {
                // Low level protocol message
                break;
            }
        }
        Debug( 4, "Format %d, chunkStreamId %d", format, chunkStreamId );

        RequestMap::iterator iter = mRequests.find( chunkStreamId );
        RtmpRequest *rtmpRequest = 0;
        if ( iter != mRequests.end() )
            rtmpRequest = iter->second;
        else
            mRequests.insert( RequestMap::value_type( chunkStreamId, rtmpRequest = new RtmpRequest ) );

        rtmpRequest->build( format, request );

        int bytesToLoad = rtmpRequest->messageLength()-rtmpRequest->payload().size();

        if ( bytesToLoad )
        {
            size_t chunkSize = min(mConnection->rxChunkSize(),request.size());
            chunkSize = min(bytesToLoad,chunkSize);

            Debug( 5, "Adding %zd bytes to chunk stream id %d", chunkSize, chunkStreamId );
            rtmpRequest->addPayload( request.data(), chunkSize );
            request -= chunkSize;
        }
    }

    for ( RequestMap::iterator iter = mRequests.begin(); iter != mRequests.end(); iter++ )
    {
        if ( iter->second->complete() )
        {
            handleRequest( iter->first, *(iter->second), response );
            iter->second->clearPayload();
        }
    }
    return( true );
}

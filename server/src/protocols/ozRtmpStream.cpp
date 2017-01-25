#include "../base/oz.h"
#include "ozRtmpStream.h"

#include "ozRtmp.h"
#include "ozRtmpAmf.h"
#include "ozRtmpSession.h"
#include "ozRtmpRequest.h"
#include "ozRtmpConnection.h"
#include "../encoders/ozH264Relay.h"
#include "../base/ozFeedFrame.h"
#include "../base/ozFfmpeg.h"

RtmpStream::RtmpStream( RtmpSession *rtmpSession, RtmpConnection *connection, int id, FeedProvider *provider, uint16_t width, uint16_t height, FrameRate frameRate, uint32_t bitRate, uint8_t quality ) :
    Stream( "RtmpStream", stringtf( "%X", rtmpSession->session() ), connection, provider ),
    Thread( identity() ),
    mRtmpSession( rtmpSession ),
    mRtmpConnection( connection ),
    mStreamId( id ),
    mH264Relay( NULL ),
    mFirstFrame( true ),
    mInitialFrames( 40 )
{
    Debug( 2, "New RTMP stream, id %d, identity %s", mStreamId, mProvider->cidentity() );

    std::string encoderKey = H264Relay::getPoolKey( mProvider->identity(), width, height, frameRate, bitRate, quality );
    if ( !(mEncoder = Encoder::getPooledEncoder( encoderKey )) )
    {
        mEncoder = mH264Relay = new H264Relay( mProvider->identity(), width, height, frameRate, bitRate, quality );
        mEncoder->registerProvider( *mProvider );
        Encoder::poolEncoder( mH264Relay );
        mH264Relay->start();
    }
    else
        mH264Relay = dynamic_cast<H264Relay *>(mEncoder);
    registerProvider( *mEncoder );
}

RtmpStream::~RtmpStream()
{
    deregisterProvider( *mEncoder );
}

ByteBuffer *RtmpStream::buildPacket( uint32_t chunkStreamStreamId, uint8_t messageTypeId, uint32_t messageStreamStreamId, uint32_t timestamp, ByteBuffer &payload, int packetType )
{
    ByteBuffer *packet = new ByteBuffer;

    size_t chunkSize = min(mRtmpConnection->txChunkSize(),payload.size());

    if ( packetType == 0x00 && (mInitialFrames > 0) )
    {
        packetType = 0x40;
        mInitialFrames--;
    }

    switch( packetType )
    {
        case 0x00 :
        {
            packet->append( chunkStreamStreamId&0x3f );

            RtmpConnection::ChunkHeader0 header;
            //header.timestamp = htobe24(mConnection->rtmpTimestamp( timestamp ));
            //Info( "Timestamp = %u", timestamp );
            header.timestamp = htobe24( 0 );
            header.messageLength = htobe24(payload.size());
            header.messageTypeId = messageTypeId;
            header.messageStreamId = htole32(mStreamId);
            packet->append( &header, sizeof(header) );

            packet->append( payload.data(), chunkSize );
            payload -= chunkSize;
            break;
        }
        case 0x40 :
        {
            packet->append( 0x40|(chunkStreamStreamId&0x3f) );

            //Info( "Timestamp = %u", timestamp );
            //uint32_t timestampDelta = timestamp-baseTimestamp;
            //baseTimestamp = timestamp;
            //Info( "Timestamp delta = %u", timestampDelta );
            uint32_t timestampDelta = timestamp;
            RtmpConnection::ChunkHeader1 header;
            header.timestampDelta = htobe24(timestampDelta);
            header.messageLength = htobe24(payload.size());
            header.messageTypeId = messageTypeId;
            packet->append( &header, sizeof(header) );

            packet->append( payload.data(), chunkSize );
            payload -= chunkSize;
            break;
        }
        case 0xc0 :
        {
            packet->append( 0xc0|(chunkStreamStreamId&0x3f) );
            packet->append( payload.data(), chunkSize );
            payload -= chunkSize;
            break;
        }
        default :
        {
            Fatal( "Unexpected packet type %02x", packetType );
        }
    }
    return( packet );
}

bool RtmpStream::sendFrame( const FramePtr &frame )
{
    Debug( 1, "Got %zd byte frame to packetise, timestamp %jd", frame->buffer().size(), frame->timestamp() );

    const unsigned char *startPos = frame->buffer().head();

    Hexdump( 2, frame->buffer().data(), frame->buffer().size()>256?256:frame->buffer().size() );

    //FIXME - Make this member data
    uint32_t timestampDelta = (1000*mH264Relay->frameRate().den)/mH264Relay->frameRate().num;
    //uint32_t timestampDelta = 50;
    if ( mFirstFrame )
    {
        ByteBuffer payload;
        payload.append( 0x17 );
        payload.append( 0x00 );
        payload.append( 0x00 );
        payload.append( 0x00 );
        payload.append( 0x00 );
        payload.append( 0x01 );
        payload.append( mH264Relay->avcProfile() );
        payload.append( 0x00 );
        payload.append( mH264Relay->avcLevel() ); // AVC Level
        payload.append( 0x03 );

        startPos = h264StartCode( startPos, frame->buffer().tail() );
                //bool firstPacketFrame = true;
        bool cuedIn = false;
        while ( startPos < frame->buffer().tail() )
        {
            while( !*(startPos++) )
                ;
            Debug( 4, "Got startpos at %zd", startPos-frame->buffer().head() );
            const unsigned char *nextStartPos = h264StartCode( startPos, frame->buffer().tail() );

            int frameSize = nextStartPos-startPos;

            unsigned char type = startPos[0] & 0x1F;
            unsigned char nri = (startPos[0] & 0x60) >> 5;
            Debug( 1, "Type %d, NRI %d (%02x), %d bytes", type, nri, startPos[0], frameSize );

            //if ( type != 7 && type != 8 && type != 6 )
            if ( type == NAL_SPS || type == NAL_PPS )
            {
                Debug( 4, "Cued in" );
                cuedIn = true;
                Hexdump( 2, startPos, frameSize );
                payload.append( 0x01 );
                uint16_t len = htobe16( frameSize );
                payload.append( &len, sizeof(len) );
                payload.append( startPos, frameSize );
            }
            else if ( cuedIn )
            {
                break;
            }
            startPos = nextStartPos;
        }
        if ( !cuedIn )
            return( false );
        while( payload.size() )
        {
            ByteBuffer *packet = buildPacket( OZ_RTMP_CSID_CONTROL, OZ_RTMP_TYPE_VIDEO, mStreamId, timestampDelta, payload, 0x00 );
            Hexdump( 2, packet->data(), min(256,packet->size()) );
            mRtmpConnection->sendResponse( static_cast<void *>(packet->data()), packet->size() );
            Debug( 1, "Sent %zd byte packet", packet->size() );
        }
        mFirstFrame = false;
    }

    if ( startPos == frame->buffer().head() )
        startPos = h264StartCode( startPos, frame->buffer().tail() );
            //bool firstPacketFrame = true;
    while ( startPos < frame->buffer().tail() )
    {
        if ( !*startPos )
            while( !*(startPos++) )
                ;
        Debug( 4, "Got startpos at %zd", startPos-frame->buffer().head() );
        const unsigned char *nextStartPos = h264StartCode( startPos, frame->buffer().tail() );

        int frameSize = nextStartPos-startPos;

        unsigned char type = startPos[0] & 0x1F;
        unsigned char nri = (startPos[0] & 0x60) >> 5;

        Debug( 1, "Type %d, NRI %d (%02x), %d bytes", type, nri, startPos[0], frameSize );

        if ( nri > 0 )
        {
            ByteBuffer payload;
            int packetType = 0xff;
            switch( nri )
            {
                case 0 :
                    packetType = 0x00;
                    break;
                case 2 :
                    packetType = 0x00;
                    break;
                case 3 :
                    packetType = 0x00;
                    break;
                default :
                    Fatal( "Unexpected NRI: %d", nri );
            }
            if ( type == 5 )
            {
                payload.append( 0x17 );
                payload.append( 0x01 );
                payload.append( 0x00 );
                payload.append( 0x00 );
                payload.append( 0x00 );
            }
            else if ( type == 1 )
            {
                payload.append( 0x27 );
                payload.append( 0x01 );
                payload.append( 0x00 );
                payload.append( 0x00 );
                payload.append( 0x00 );
            }
            if ( payload.size() )
            {
                uint32_t len = htobe32( frameSize );
                payload.append( &len, sizeof(len) );
                payload.append( startPos, frameSize );

                while( payload.size() )
                {
                    ByteBuffer *packet = buildPacket( OZ_RTMP_CSID_CONTROL, OZ_RTMP_TYPE_VIDEO, mStreamId, timestampDelta, payload, packetType );
                    Hexdump( 2, packet->data(), min(256,packet->size()) );
                    mRtmpConnection->sendResponse( static_cast<void *>(packet->data()), packet->size() );
                    Debug( 1, "Sent %zd byte packet", packet->size() );
                    packetType = 0xc0;
                }
            }
        }
        startPos = nextStartPos;
    }
    return( true );
}

int RtmpStream::run()
{
    Select select( 60 );
    select.addWriter( mConnection->socket() );

    if ( !waitForProviders() )
        return( 1 );

    ByteBuffer response;
    if ( false )
    {
        Amf0Record amfNotify( "|RtmpSampleAccess" );
        Amf0Record amfData1( false );
        Amf0Record amfData2( false );

        ByteBuffer payload;
        amfNotify.encode( payload );
        amfData1.encode( payload );
        amfData2.encode( payload );

        mRtmpSession->buildResponse0( response, OZ_RTMP_CSID_CONTROL, OZ_RTMP_TYPE_DATA_AMF0, mStreamId, payload );
        Debug( 2, "Built Command Message - %s", amfNotify.getString().c_str() );
    }
    if ( true )
    {
        Amf0Record amfNotify( "onMetaData" );
        Amf0Object amfData;
        amfData.addRecord( "author", Amf0Record( "" ) );
        amfData.addRecord( "copyright", Amf0Record( "" ) );
        amfData.addRecord( "description", Amf0Record( "" ) );
        amfData.addRecord( "keywords", Amf0Record( "" ) );
        amfData.addRecord( "rating", Amf0Record( "" ) );
        amfData.addRecord( "title", Amf0Record( "" ) );
        amfData.addRecord( "presetName", Amf0Record( "Custom" ) );
        time_t now = time( 0 );
        char timeString[32] = "";
        strftime( timeString, sizeof(timeString), "%a %b %d %H:%M:%S %Y", localtime( &now ) );
        amfData.addRecord( "creationDate", Amf0Record( timeString ) );
        amfData.addRecord( "videoDevice", Amf0Record( "USIOne Camera" ) );
        amfData.addRecord( "frameRate", Amf0Record( (int)mH264Relay->frameRate() ) );
        amfData.addRecord( "width", Amf0Record( mH264Relay->width() ) );
        amfData.addRecord( "height", Amf0Record( mH264Relay->height() ) );
        amfData.addRecord( "videocodecid", Amf0Record( "avc1" ) );
        amfData.addRecord( "videodatarate", Amf0Record( (int)mH264Relay->bitRate() ) );
        amfData.addRecord( "avclevel", Amf0Record( mH264Relay->avcLevel() ) );
        amfData.addRecord( "avcprofile", Amf0Record( mH264Relay->avcProfile() ) );
        amfData.addRecord( "videokeyframe_frequency", Amf0Record( int(mH264Relay->gopSize()/mH264Relay->frameRate().toInt() ) ) );

        ByteBuffer payload;
        amfNotify.encode( payload );
        amfData.encode( payload );

        mRtmpSession->buildResponse0( response, OZ_RTMP_CSID_CONTROL, OZ_RTMP_TYPE_DATA_AMF0, mStreamId, payload );
        Debug( 2, "Built Command Message - %s", amfNotify.getString().c_str() );
    }

    mRtmpConnection->sendResponse( static_cast<void *>(response.data()), response.size() );
    response.clear();

    while ( !mStop && select.wait() >= 0 )
    {
        if ( mStop )
           break;
        mQueueMutex.lock();
        Select::CommsList writeable = select.getWriteable();
        if ( writeable.size() <= 0 )
        {
            Error( "Writer timed out" );
            mStop = true;
            break;
        }
        if ( !mFrameQueue.empty() )
        {
            bool sendToSocket = false;
            for ( Select::CommsList::iterator iter = writeable.begin(); iter != writeable.end(); iter++ )
                if ( TcpInetSocket *socket = dynamic_cast<TcpInetSocket *>(*iter) )
                    if ( socket == mConnection->socket() )
                        sendToSocket = true;
            if ( sendToSocket )
            {
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    sendFrame( *iter );
                }
                mFrameQueue.clear();
            }
        }
        mQueueMutex.unlock();
        usleep( INTERFRAME_TIMEOUT );
    }

    return( 0 );
}

#include "../base/oz.h"
#include "ozRtspStream.h"

#include "ozRtsp.h"
#include "ozRtspConnection.h"
#include "ozRtspSession.h"
#include "ozRtpSession.h"
#include "ozRtpCtrl.h"
#include "ozRtpData.h"
#include "../encoders/ozH264Encoder.h"
#include "../encoders/ozH264Relay.h"
#include "../encoders/ozMpegEncoder.h"
#include "../base/ozFeedFrame.h"
#include "../base/ozFfmpeg.h"

/**
* @brief 
*
* @param rtspSession
* @param trackId
* @param connection
* @param encoder
* @param transport
* @param profile
* @param lowerTransport
* @param transportParms
*/
RtspStream::RtspStream( RtspSession *rtspSession, int trackId, RtspConnection *connection, Encoder *encoder, const std::string &transport, const std::string &profile, const std::string &lowerTransport, const StringTokenList::TokenList &transportParms ) :
    Stream( "RtspStream", stringtf( "%X-%d ", rtspSession->session(), trackId ), connection, encoder ),
    Thread( identity() ),
    mRtspSession( rtspSession ),
    mTrackId( trackId ),
    mRtspConnection( connection ),
    mSsrc( 0 ),
    mSsrc1( 0 ),
    mFormatContext( 0 ),
    mRtpTime( 0 )
{
    Debug( 2, "New RTSP stream %d", trackId );

    if ( transport != "RTP" )
        throw RtspException( stringtf( "Unrecognised RTSP transport: %s", transport.c_str() ) );
    mTransport = TRANS_RTP;

    if ( profile != "AVP" )
        throw RtspException( stringtf( "Unrecognised RTSP profile: %s", profile.c_str() ) );
    mProfile = PROF_AVP;

    if ( lowerTransport == "UDP" )
        mLowerTransport = LOWTRANS_UDP;
    else if ( lowerTransport == "TCP" )
        mLowerTransport = LOWTRANS_TCP;
    else
        throw RtspException( stringtf( "Unrecognised RTSP lower transport: %s", lowerTransport.c_str() ) );

    mDelivery = DEL_MULTICAST;
    if ( mRtspConnection->getRemoteAddr() )
        mDestAddr = mRtspConnection->getRemoteAddr()->host();
    mDestPorts[0] = -1;
    mDestPorts[1] = -1;
    mSourcePorts[0] = -1;
    mSourcePorts[1] = -1;
    mInterleaved = false;
    mChannels[0] = 0;
    mChannels[1] = 1;
    mTtl = 0;

    for ( int i = 0; i < transportParms.size(); i++ )
    {
        const std::string &transportPart = transportParms[i];
        StringTokenList exprParts( transportPart, "=" );
        if ( exprParts.size() <= 0 || exprParts.size() > 2 )
            throw RtspException( stringtf( "Unable to parse RTSP transport specification: %s", transportPart.c_str() ) );
        const std::string &name = exprParts[0];
        std::string value;
        if ( exprParts.size() > 1 )
            value = exprParts[1];
            
        if ( name == "unicast" )
        {
            mDelivery = DEL_UNICAST;
        }
        else if ( name == "multicast" )
        {
            mDelivery = DEL_MULTICAST;
        }
        else if ( name == "destination" )
        {
            mDestAddr = value;;
        }
        else if ( name == "ssrc" )
        {
            mSsrc = strtol( value.c_str(), NULL, 10 );
        }
        else if ( name == "interleaved" )
        {
            mInterleaved = true;
            StringTokenList ports( value, "-" );
            if ( ports.size() <= 0 || ports.size() > 2 )
                throw RtspException( stringtf( "Unable to parse RTSP transport port specification: %s", value.c_str() ) );
            mDestPorts[0] = strtol( ports[0].c_str(), NULL, 10 );
            mDestPorts[1] = strtol( ports[1].c_str(), NULL, 10 );
        }
        else if ( name == "ttl" )
        {
            mTtl = strtol( value.c_str(), NULL, 10 );
        }
        else if ( name == "port" )
        {
            StringTokenList ports( value, "-" );
            if ( ports.size() <= 0 || ports.size() > 2 )
                throw RtspException( stringtf( "Unable to parse RTSP transport port specification: %s", value.c_str() ) );
            mDestPorts[0] = strtol( ports[0].c_str(), NULL, 10 );
            mDestPorts[1] = strtol( ports[1].c_str(), NULL, 10 );
        }
        else if ( name == "client_port" )
        {
            StringTokenList ports( value, "-" );
            if ( ports.size() <= 0 || ports.size() > 2 )
                throw RtspException( stringtf( "Unable to parse RTSP transport client_port specification: %s", value.c_str() ) );
            mDestPorts[0] = strtol( ports[0].c_str(), NULL, 10 );
            mDestPorts[1] = strtol( ports[1].c_str(), NULL, 10 );
        }
        //else if ( name == "server_port" )
        //{
            //StringTokenList ports( value, "-" );
            //if ( ports.size() <= -2 || ports.size() > 2 )
                //throw RtspException( stringtf( "Unable to parse RTSP transport server_port specification: %s", value.c_str() ) );
            //mSourcePorts[0] = strtol( ports[0].c_str(), NULL, 10 );
            //mSourcePorts[1] = strtol( ports[1].c_str(), NULL, 10 );
        //}
        else if ( name == "mode" )
        {
            if ( value != "PLAY" )
                throw RtspException( stringtf( "Unrecognised RTSP Mode: %s", transportPart.c_str() ) );
        }
        else
        {
            Debug( 3, "Ignoring transport part '%s'", transportPart.c_str() );
        }
    }
    if ( !mInterleaved )
    {
        mSourcePorts[0] = mRtspSession->requestPorts();
        mSourcePorts[1] = mSourcePorts[0]+1;
    }

    mSsrc = mRtspSession->requestSsrc();
    Debug( 3, "SSRC: %08X", mSsrc );

    mRtpSession = new RtpSession( mSsrc, "localhost", mSourcePorts[0], mDestAddr, mDestPorts[0] );
    mRtpCtrl = new RtpCtrlManager( *this, *mRtpSession );
    mRtpData = new RtpDataManager( *this, *mRtpSession, 96, mEncoder->frameRate(), (AVRational){1,90000} );

    registerProvider( *mEncoder );
}

/**
* @brief 
*/
RtspStream::~RtspStream()
{
    deregisterProvider( *mEncoder );

    delete mRtpData;
    delete mRtpCtrl;
    delete mRtpSession;

    if ( !mInterleaved )
        mRtspSession->releasePorts(mSourcePorts[0]);
    mRtspSession->releaseSsrc( mSsrc );
}

bool ready = false;

// Write the encoded H.264 frameout to RTP. Not quite as simple as that as need to mess with the RTP
// NAL (Network Access Layer) as well.
/**
* @brief 
*
* @param frame
*/
void RtspStream::packetiseFrame( const FramePtr &frame )
{
    Debug( 1, "Got %zd byte frame to packetise", frame->buffer().size() );

    if ( mEncoder->cl4ss() == "H264Encoder" || mEncoder->cl4ss() == "H264Relay" )
    {
    const unsigned char *startPos = h264StartCode( frame->buffer().head(), frame->buffer().tail() );
    while ( startPos < frame->buffer().tail() )
    {
        while( !*(startPos++) )
            ;
        const unsigned char *nextStartPos = h264StartCode( startPos, frame->buffer().tail() );

        //nal_send(s1, r, r1 - r, (r1 == buf1 + size));
        //static void nal_send(AVFormatContext *s1, const uint8_t *buf, int size, int last)

        int frameSize = nextStartPos-startPos;

        if ( frameSize <= OZ_RTP_MAX_PACKET_SIZE )
        {
            unsigned char type = startPos[0] & 0x1F;
            unsigned char nri = startPos[0] & 0x60;

            Debug( 2, "Packet type %d, nri %0x, size %d", type, nri, frameSize );

            //ff_rtp_send_data(s1, buf, size, last);
            mRtpData->buildPacket( startPos, frameSize, frame->timestamp(), (nextStartPos == frame->buffer().tail()) );
        }
        else
        {
            unsigned char tempBuffer[OZ_RTP_MAX_PACKET_SIZE];

            unsigned char type = startPos[0] & 0x1F;
            unsigned char nri = startPos[0] & 0x60;

            Debug( 2, "Packet type %d, nri %0x, size %d", type, nri, frameSize );

            tempBuffer[0] = 28;        /* FU Indicator; Type = 28 ---> FU-A */
            tempBuffer[0] |= nri;
            tempBuffer[1] = type;
            tempBuffer[1] |= 1 << 7;

            startPos += 1;
            frameSize -= 1;
            while ( (frameSize + 2) > OZ_RTP_MAX_PACKET_SIZE )
            {
                memcpy( &tempBuffer[2], startPos, OZ_RTP_MAX_PACKET_SIZE - 2);
                mRtpData->buildPacket( tempBuffer, OZ_RTP_MAX_PACKET_SIZE, frame->timestamp(), false );
                startPos += OZ_RTP_MAX_PACKET_SIZE - 2;
                frameSize -= OZ_RTP_MAX_PACKET_SIZE - 2;
                tempBuffer[1] &= ~(1 << 7);
            }
            tempBuffer[1] |= 1 << 6;
            memcpy( &tempBuffer[2], startPos, frameSize );
            mRtpData->buildPacket( tempBuffer, frameSize+2, frame->timestamp(), (nextStartPos == frame->buffer().tail()) );
        }
        startPos = nextStartPos;
    }
    }
    else
    {
        if ( frame->buffer().size() <= OZ_RTP_MAX_PACKET_SIZE )
        {
            //ff_rtp_send_data(s1, buf, size, last);
            mRtpData->buildPacket( frame->buffer().data(), frame->buffer().size(), frame->timestamp(), true );
        }
        else
        {
            unsigned char tempBuffer[OZ_RTP_MAX_PACKET_SIZE];
            uint8_t *frameDataPtr = frame->buffer().data();
            uint32_t frameSize = frame->buffer().size();
            while ( frameSize > OZ_RTP_MAX_PACKET_SIZE )
            {
                memcpy( tempBuffer, frameDataPtr, OZ_RTP_MAX_PACKET_SIZE );
                mRtpData->buildPacket( tempBuffer, OZ_RTP_MAX_PACKET_SIZE, frame->timestamp(), false );
                frameDataPtr += OZ_RTP_MAX_PACKET_SIZE;
                frameSize -= OZ_RTP_MAX_PACKET_SIZE;
            }
            memcpy( tempBuffer, frameDataPtr, frameSize );
            mRtpData->buildPacket( tempBuffer, frameSize, frame->timestamp(), true );
        }
    }
}

/**
* @brief 
*
* @return 
*/
int RtspStream::run()
{
    Debug( 3, "Dest address = %s", mDestAddr.c_str() );

    if ( !waitForProviders() )
        return( 1 );

    // Set up the local and remote addresses and sockets
    if ( !mInterleaved )
    {
        SockAddrInet localDataAddr;
        SockAddrInet localCtrlAddr;
        SockAddrInet remoteDataAddr;
        SockAddrInet remoteCtrlAddr;

        UdpInetServer rtpDataSocket;
        UdpInetServer rtpCtrlSocket;

        localDataAddr.resolve( mSourcePorts[0], "udp" );
        if ( !rtpDataSocket.bind( localDataAddr ) )
            Fatal( "Failed to bind RTP data socket" );
        Debug( 3, "Bound RTP data to :%d", mSourcePorts[0] );
        localCtrlAddr.resolve( mSourcePorts[1], "udp" );
        if ( !rtpCtrlSocket.bind( localCtrlAddr ) )
            Fatal( "Failed to bind RTP ctrl socket" );
        Debug( 3, "Bound RTP ctrl to :%d", mSourcePorts[1] );

        Debug( 3, "Connecting RTP data to %s:%d", mDestAddr.c_str(), mDestPorts[0] );
        remoteDataAddr.resolve( mDestAddr.c_str(), mDestPorts[0], "udp" );
        if ( !rtpDataSocket.connect( remoteDataAddr ) )
            Fatal( "Failed to connect RTP data socket" );
        Debug( 3, "Connecting RTP ctrl to %s:%d", mDestAddr.c_str(), mDestPorts[1] );
        remoteCtrlAddr.resolve( mDestAddr.c_str(), mDestPorts[1], "udp" );
        if ( !rtpCtrlSocket.connect( remoteCtrlAddr ) )
            Fatal( "Failed to connect RTP ctrl socket" );

        // This timeout selected originally in case wanted to kill stream if no Receiver Report
        // received. As we now have a writer on the select will never get to that time.
        Select select( 60 );
        select.addReader( &rtpCtrlSocket );
        select.addWriter( &rtpDataSocket );

        unsigned char buffer[BUFSIZ];
        while ( !mStop && select.wait() >= 0 )
        {
            if ( mStop )
                break;
            Select::CommsList writeable = select.getWriteable();
            Select::CommsList readable = select.getReadable();
            //Info( "rS: %zd, wS: %zd", readable.size(), writeable.size() );
            if ( writeable.size() > 0 )
            {
                mQueueMutex.lock();
                if ( !mFrameQueue.empty() )
                {
                    for ( FrameQueue::iterator frameIter = mFrameQueue.begin(); frameIter != mFrameQueue.end(); frameIter++ )
                    {
                        packetiseFrame( *frameIter );
                        //delete *frameIter;
                    }
                    mFrameQueue.clear();
                }
                mQueueMutex.unlock();

                mRtpData->lockPacketQueue();
                PacketQueue &packetQueue = mRtpData->packetQueue();
                if ( !packetQueue.empty() )
                {
                    bool sendToSocket = false;
                    for ( Select::CommsList::iterator socketIter = writeable.begin(); socketIter != writeable.end(); socketIter++ )
                        if ( UdpInetServer *socket = dynamic_cast<UdpInetServer *>(*socketIter) )
                            if ( socket == &rtpDataSocket )
                                sendToSocket = true;
                    if ( sendToSocket )
                    {
                        for ( PacketQueue::iterator packetIter = packetQueue.begin(); packetIter != packetQueue.end(); packetIter++ )
                        {
                            const ByteBuffer *packet = *packetIter;
                            /*int nBytes = */rtpDataSocket.write( packet->data(), packet->size() );
                            Debug( 5, "Wrote %zd byte packet to socket", packet->size() );
                            delete *packetIter;
                        }
                        packetQueue.clear();
                    }
                }
                mRtpData->unlockPacketQueue();
            }
            else if ( readable.size() == 0 )
            {
                Error( "RTP timed out" );
                mStop = true;
                break;
            }
            for ( Select::CommsList::iterator iter = readable.begin(); iter != readable.end(); iter++ )
            {
                if ( UdpInetServer *socket = dynamic_cast<UdpInetServer *>(*iter) )
                {
                    int nBytes = socket->recv( buffer, sizeof(buffer) );
                    Debug( 4, "Read %d bytes on sd %d", nBytes, socket->getReadDesc() );
                    if ( nBytes )
                    {
                         // Process RTP control packets
                         //recvPacket( buffer, nBytes );
                    }
                    else
                    {
                        mStop = true;
                        break;
                    }
                }
                else
                {
                    Panic( "The impossible has happened" );
                }
            }
            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
        rtpCtrlSocket.close();
        rtpDataSocket.close();
    }
    else
    {
        while ( !mStop )
        {
            if ( mRtspConnection->socket()->isOpen() )
            {
                mQueueMutex.lock();
                if ( !mFrameQueue.empty() )
                {
                    for ( FrameQueue::iterator frameIter = mFrameQueue.begin(); frameIter != mFrameQueue.end(); frameIter++ )
                    {
                        packetiseFrame( *frameIter );
                        //delete *frameIter;
                    }
                    mFrameQueue.clear();

                }
                mQueueMutex.unlock();

                mRtpData->lockPacketQueue();
                PacketQueue &packetQueue = mRtpData->packetQueue();
                for ( PacketQueue::iterator packetIter = packetQueue.begin(); packetIter != packetQueue.end(); packetIter++ )
                {
                    const ByteBuffer *packet = *packetIter;
                    if ( mRtspConnection->sendInterleavedPacket( mChannels[0], *packet ) )
                    {
                        Debug( 4, "Wrote %zd byte packet on channel %d", packet->size(), mChannels[0] );
                    }
                    else
                    {
                        Error( "Unable to write interleaved packet on channel %d", mChannels[0] );
                        mStop = true;
                        break;
                    }
                    delete *packetIter;
                }
                packetQueue.clear();
                mRtpData->unlockPacketQueue();
            }
            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
    }

    return( 0 );
}

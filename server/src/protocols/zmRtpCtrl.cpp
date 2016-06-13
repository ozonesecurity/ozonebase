#include "../base/zm.h"
#include "zmRtpCtrl.h"

#include "zmRtspStream.h"
#include "zmRtpSession.h"
#include "../libgen/libgenComms.h"
#include "../libgen/libgenTime.h"

/**
* @brief 
*
* @param rtspStream
* @param rtpSession
*/
RtpCtrlManager::RtpCtrlManager( RtspStream &rtspStream, RtpSession &rtpSession ) :
    mRtspStream( rtspStream ),
    mRtpSession( rtpSession )
{
}

/**
* @brief 
*/
RtpCtrlManager::~RtpCtrlManager()
{
    mQueueMutex.lock();
    for ( PacketQueue::iterator iter = mPacketQueue.begin(); iter != mPacketQueue.end(); iter++ )
        delete *iter;
    mPacketQueue.clear();
    mQueueMutex.unlock();
}

/**
* @brief 
*
* @param packet
* @param packetLen
*
* @return 
*/
int RtpCtrlManager::recvPacket( const unsigned char *packet, ssize_t packetLen )
{
    const RtcpPacket *rtcpPacket;
    rtcpPacket = (RtcpPacket *)packet;

    int consumed = 0;

    //printf( "C: " );
    //for ( int i = 0; i < packetLen; i++ )
        //printf( "%02x ", (unsigned char)packet[i] );
    //printf( "\n" );
    int ver = rtcpPacket->header.version;
    int count = rtcpPacket->header.count;
    int pt = rtcpPacket->header.pt;
    int len = ntohs(rtcpPacket->header.lenN);

    Debug( 5, "RTCP Ver: %d", ver );
    Debug( 5, "RTCP Count: %d", count );
    Debug( 5, "RTCP Pt: %d", pt );
    Debug( 5, "RTCP len: %d", len );

    switch( pt )
    {
        case RTCP_SR :
        {
            uint32_t ssrc = ntohl(rtcpPacket->body.sr.ssrcN);

            Debug( 5, "RTCP Got SR (%x)", ssrc );
            if ( !mRtspStream.updateSsrc( ssrc ) )
            {
                Warning( "Discarding packet for unrecognised ssrc %x", ssrc );
                return( -1 );
            }

            if ( len > 1 )
            {
                //printf( "NTPts:%d.%d, RTPts:%d\n", $ntptsmsb, $ntptslsb, $rtpts );
                uint16_t ntptsmsb = ntohl(rtcpPacket->body.sr.ntpSecN);
                uint16_t ntptslsb = ntohl(rtcpPacket->body.sr.ntpFracN);
                //printf( "NTPts:%x.%04x, RTPts:%x\n", $ntptsmsb, $ntptslsb, $rtpts );
                //printf( "Pkts:$sendpkts, Octs:$sendocts\n" );
                uint32_t rtpTime = ntohl(rtcpPacket->body.sr.rtpTsN);

                mRtpSession.updateRtcpData( ntptsmsb, ntptslsb, rtpTime );
            }

            unsigned char buffer[256];
            unsigned char *bufferPtr = buffer;
            bufferPtr += generateRr( bufferPtr, sizeof(buffer)-(bufferPtr-buffer) );
            bufferPtr += generateSdes( bufferPtr, sizeof(buffer)-(bufferPtr-buffer) );
            ByteBuffer *packet = new ByteBuffer( buffer, bufferPtr-buffer );
            mQueueMutex.lock();
            mPacketQueue.push_back( packet );
            Debug( 1, "Wrote %zd byteRTP ctrl packet to queue, size = %zd", packet->size(), mPacketQueue.size() );
            mQueueMutex.unlock();
            break;
        }
        case RTCP_SDES :
        {
            ssize_t contentLen = packetLen - sizeof(rtcpPacket->header);
            while ( contentLen )
            {
                Debug( 5, "RTCP CL: %zd", contentLen );
                uint32_t ssrc = ntohl(rtcpPacket->body.sdes.srcN);

                Debug( 5, "RTCP Got SDES (%x), %d items", ssrc, count );
                if ( mRtpSession.ssrc() && (ssrc != mRtpSession.ssrc()) )
                {
                    Warning( "Discarding packet for unrecognised ssrc %x", ssrc );
                    return( -1 );
                }

                unsigned char *sdesPtr = (unsigned char *)&rtcpPacket->body.sdes.item;
                for ( int i = 0; i < count; i++ )
                {
                    RtcpSdesItem *item = (RtcpSdesItem *)sdesPtr;
                    Debug( 5, "RTCP Item length %d", item->len );
                    switch( item->type )
                    {
                        case RTCP_SDES_CNAME :
                        {
                            std::string cname( item->data, item->len );
                            Debug( 5, "RTCP Got CNAME %s", cname.c_str() );
                            break;
                        }
                        case RTCP_SDES_END :
                        case RTCP_SDES_NAME :
                        case RTCP_SDES_EMAIL :
                        case RTCP_SDES_PHONE :
                        case RTCP_SDES_LOC :
                        case RTCP_SDES_TOOL :
                        case RTCP_SDES_NOTE :
                        case RTCP_SDES_PRIV :
                        default :
                        {
                            Error( "Received unexpected SDES item type %d, ignoring", item->type );
                            return( -1 );
                        }
                    }
                    int paddedLen = 4+2+item->len+1; // Add null byte
                    paddedLen = (((paddedLen-1)/4)+1)*4;
                    Debug( 5, "RTCP PL:%d", paddedLen );
                    sdesPtr += paddedLen;
                    contentLen -= paddedLen;
                }
            }
            break;
        }
        case RTCP_BYE :
        {
            Debug( 5, "RTCP Got BYE" );
            mRtspStream.stop();
            //mStop = true;
            break;
        }
        case RTCP_RR :
        case RTCP_APP :
        default :
        {
            Error( "Received unexpected packet type %d, ignoring", pt );
            return( -1 );
        }
    }
    consumed = sizeof(uint32_t)*(len+1);
    return( consumed );
}

/**
* @brief 
*
* @param packet
* @param packetLen
*
* @return 
*/
int RtpCtrlManager::generateRr( const unsigned char *packet, ssize_t packetLen )
{
    RtcpPacket *rtcpPacket = (RtcpPacket *)packet;

    int byteLen = sizeof(rtcpPacket->header)+sizeof(rtcpPacket->body.rr)+sizeof(rtcpPacket->body.rr.rr[0]);
    int wordLen = ((byteLen-1)/sizeof(uint32_t))+1;

    rtcpPacket->header.version = ZM_RTP_VERSION;
    rtcpPacket->header.p = 0;
    rtcpPacket->header.pt = RTCP_RR;
    rtcpPacket->header.count = 1;
    rtcpPacket->header.lenN = htons(wordLen-1);

    mRtpSession.updateRtcpStats();

    Debug( 5, "Ssrc = %d", mRtspStream.ssrc() );
    Debug( 5, "Ssrc_1 = %d", mRtpSession.ssrc() ); // This should be different than previous line
    Debug( 5, "Last Seq = %d", mRtpSession.maxSeq() );
    Debug( 5, "Jitter = %d", mRtpSession.jitter() );
    Debug( 5, "Last SR = %d", mRtpSession.lastSrTimestamp() );

    rtcpPacket->body.rr.ssrcN = htonl(mRtspStream.ssrc());
    rtcpPacket->body.rr.rr[0].ssrcN = htonl(mRtpSession.ssrc()); // This should be different than previous line
    rtcpPacket->body.rr.rr[0].lost = mRtpSession.lostPackets();
    rtcpPacket->body.rr.rr[0].fraction = mRtpSession.lostFraction();
    rtcpPacket->body.rr.rr[0].lastSeqN = htonl(mRtpSession.maxSeq());
    rtcpPacket->body.rr.rr[0].jitterN = htonl(mRtpSession.jitter());
    rtcpPacket->body.rr.rr[0].lsrN = htonl(mRtpSession.lastSrTimestamp());
    rtcpPacket->body.rr.rr[0].dlsrN = 0;

    return( wordLen*sizeof(uint32_t) );
}

/**
* @brief 
*
* @param packet
* @param packetLen
*
* @return 
*/
int RtpCtrlManager::generateSdes( const unsigned char *packet, ssize_t packetLen )
{
    RtcpPacket *rtcpPacket = (RtcpPacket *)packet;

    const std::string &cname = mRtpSession.getCname();

    int byteLen = sizeof(rtcpPacket->header)+sizeof(rtcpPacket->body.sdes)+sizeof(rtcpPacket->body.sdes.item[0])+cname.size();
    int wordLen = ((byteLen-1)/sizeof(uint32_t))+1;

    rtcpPacket->header.version = ZM_RTP_VERSION;
    rtcpPacket->header.p = 0;
    rtcpPacket->header.pt = RTCP_SDES;
    rtcpPacket->header.count = 1;
    rtcpPacket->header.lenN = htons(wordLen-1);

    rtcpPacket->body.sdes.srcN = htonl(mRtpSession.ssrc());
    rtcpPacket->body.sdes.item[0].type = RTCP_SDES_CNAME;
    rtcpPacket->body.sdes.item[0].len = cname.size();
    memcpy( rtcpPacket->body.sdes.item[0].data, cname.data(), cname.size() );

    return( wordLen*sizeof(uint32_t) );
}

/**
* @brief 
*
* @param packet
* @param packetLen
*
* @return 
*/
int RtpCtrlManager::generateBye( const unsigned char *packet, ssize_t packetLen )
{
    RtcpPacket *rtcpPacket = (RtcpPacket *)packet;

    int byteLen = sizeof(rtcpPacket->header)+sizeof(rtcpPacket->body.bye)+sizeof(rtcpPacket->body.bye.srcN[0]);
    int wordLen = ((byteLen-1)/sizeof(uint32_t))+1;

    rtcpPacket->header.version = ZM_RTP_VERSION;
    rtcpPacket->header.p = 0;
    rtcpPacket->header.pt = RTCP_BYE;
    rtcpPacket->header.count = 1;
    rtcpPacket->header.lenN = htons(wordLen-1);

    rtcpPacket->body.bye.srcN[0] = htonl(mRtpSession.ssrc());

    return( wordLen*sizeof(uint32_t) );
}

/**
* @brief 
*
* @param buffer
* @param nBytes
*
* @return 
*/
int RtpCtrlManager::recvPackets( unsigned char *buffer, ssize_t nBytes )
{
    unsigned char *bufferPtr = buffer;

    // u_int32 len;        /* length of compound RTCP packet in words */
    // rtcp_t *r;          /* RTCP header */
    // rtcp_t *end;        /* end of compound RTCP packet */

    // if ((*(u_int16 *)r & RTCP_VALID_MASK) != RTCP_VALID_VALUE) {
        // /* something wrong with packet format */
    // }
    // end = (rtcp_t *)((u_int32 *)r + len);

    // do r = (rtcp_t *)((u_int32 *)r + r->common.length + 1);
    // while (r < end && r->common.version == 2);

    // if (r != end) {
        // /* something wrong with packet format */
    // }

    while ( nBytes > 0 )
    {
        int consumed = recvPacket( bufferPtr, nBytes );
        if ( consumed <= 0 )
            break;
        bufferPtr += consumed;
        nBytes -= consumed;
    }
    return( nBytes );
}


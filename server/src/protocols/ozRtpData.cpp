#include "../base/oz.h"
#include "ozRtpData.h"

#include "ozRtpSession.h"
#include "../libgen/libgenComms.h"

/**
* @brief 
*
* @param rtspStream
* @param rtpSession
* @param payloadType
* @param frameRate
* @param clockRate
*/
RtpDataManager::RtpDataManager( RtspStream &rtspStream, RtpSession &rtpSession, uint8_t payloadType, const AVRational &frameRate, const AVRational &clockRate ) :
    mRtspStream( rtspStream ),
    mRtpSession( rtpSession ),
    mPayloadType( payloadType ),
    mFrameRate( frameRate ),
    mClockRate( clockRate ),
    mTimestampBase( 0 ),
    mSeq( 0 )
{
}

/**
* @brief 
*/
RtpDataManager::~RtpDataManager()
{
    mQueueMutex.lock();
    for ( PacketQueue::iterator iter = mPacketQueue.begin(); iter != mPacketQueue.end(); iter++ )
        delete *iter;
    mPacketQueue.clear();
    mQueueMutex.unlock();
}

// Assemble an RTP packet
/**
* @brief 
*
* @param data
* @param size
* @param timecode
* @param last
*/
void RtpDataManager::buildPacket( const unsigned char *data, int size, uint32_t timecode, bool last )
{
    if ( !mTimestampBase )
        mTimestampBase = timecode;
    uint32_t timestamp = timecode - mTimestampBase;
    //uint32_t timestamp = av_rescale_q( timecode - mTimestampBase, mFrameRate, mClockRate );
#if 0
    uint32_t timestampDelta = (timecode - mTimestampBase);
    double timestampFactor = (double)mClockRate.den/1000000.0;
    Debug( 6, "Packet QTSd = %d", timestampDelta );
    uint32_t timestamp = timestampDelta*timestampFactor;
    Debug( 6, "Packet QTS = %d", timestamp );
#endif

    RtpDataHeader rtpHeader;
    rtpHeader.version = ZM_RTP_VERSION;
    rtpHeader.p = 0; // Padding
    rtpHeader.x = 0; // Extension
    rtpHeader.cc = 0; // CCRC Count
    rtpHeader.m = last; // Marker
    rtpHeader.pt = mPayloadType;
    rtpHeader.seqN = htons(mSeq);
    rtpHeader.timestampN = htonl(timestamp);
    rtpHeader.ssrcN = ntohl(mRtpSession.ssrc());

    ByteBuffer *packet = new ByteBuffer( sizeof(RtpDataHeader)+size );

    packet->assign( reinterpret_cast<unsigned char *>( &rtpHeader ), sizeof(rtpHeader) );
    packet->append( data, size );

    mSeq++;

    mQueueMutex.lock();
    mPacketQueue.push_back( packet );
    Debug( 5, "Wrote %zd byte RTP data packet to queue, size = %zd", packet->size(), mPacketQueue.size() );
    mQueueMutex.unlock();
}

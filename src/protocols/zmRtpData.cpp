#include "../zm.h"
#include "zmRtpData.h"

#include "zmRtpSession.h"
#include "../libgen/libgenComms.h"

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

RtpDataManager::~RtpDataManager()
{
    mQueueMutex.lock();
    for ( PacketQueue::iterator iter = mPacketQueue.begin(); iter != mPacketQueue.end(); iter++ )
        delete *iter;
    mPacketQueue.clear();
    mQueueMutex.unlock();
}

// Assemble an RTP packet
void RtpDataManager::buildPacket( const unsigned char *data, int size, uint32_t timecode, bool last )
{
    if ( !mTimestampBase )
        mTimestampBase = timecode;
    uint32_t timestamp = timecode-mTimestampBase;
    //uint32_t timestamp = av_rescale_q( frame.timestamp - mTimestampBase, mFrameRate, mClockRate );
    //uint32_t timestampDelta = (timecode - mTimestampBase);
    //double timestampFactor = (double)mClockRate.den/1000000.0;
    //Debug( 6, "Packet QTSd = %d", timestampDelta );
    //uint32_t timestamp = timestampDelta*timestampFactor;
    //Debug( 6, "Packet QTS = %d", timestamp );

    RtpDataHeader rtpHeader;
    rtpHeader.version = ZM_RTP_VERSION;
    rtpHeader.p = 0; // Padding
    rtpHeader.x = 0; // Extension
    rtpHeader.cc = 0; // CCRC Count
    rtpHeader.m = last; // Marker
    rtpHeader.pt = mPayloadType;
    rtpHeader.seqN = htons(mSeq);
    rtpHeader.timestampN = htonl(timecode);
    rtpHeader.ssrcN = ntohl(mRtpSession.ssrc());

    ByteBuffer *packet = new ByteBuffer( sizeof(RtpDataHeader)+size );

    packet->assign( reinterpret_cast<unsigned char *>( &rtpHeader ), sizeof(rtpHeader) );
    packet->append( data, size );

    mSeq++;

    mQueueMutex.lock();
    mPacketQueue.push_back( packet );
    Debug( 1, "Wrote %zd byte RTP data packet to queue, size = %zd", packet->size(), mPacketQueue.size() );
    mQueueMutex.unlock();
}

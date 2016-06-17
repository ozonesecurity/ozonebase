#include "../base/oz.h"
#include "ozRtpSession.h"

#include "../libgen/libgenTime.h"
#include "ozRtpData.h"

//#include <arpa/inet.h>

/**
* @brief 
*
* @param ssrc
* @param localHost
* @param localPortBase
* @param remoteHost
* @param remotePortBase
*/
RtpSession::RtpSession( uint32_t ssrc, const std::string &localHost, int localPortBase, const std::string &remoteHost, int remotePortBase ) :
    mSsrc( ssrc ),
    mLocalHost( localHost ),
    mRemoteHost( remoteHost ),
    mRtpClock( 0 ),
    mFrameCount( 0 )
{
    int seq = 0;

    char hostname[256] = "";
    gethostname( hostname, sizeof(hostname) );

    mCname = stringtf( "CaMeRa@%s", hostname );
    Debug( 3, "RTP CName = %s", mCname.c_str() );

    init( seq );
    mMaxSeq = seq - 1;
    mProbation = MIN_SEQUENTIAL;

    mLocalPortChans[0] = localPortBase;
    mLocalPortChans[1] = localPortBase+1;

    mRemotePortChans[0] = remotePortBase;
    mRemotePortChans[1] = remotePortBase+1;

    mBaseTimeReal = tvNow();
    mBaseTimeNtp = tvZero();
    mBaseTimeRtp = 0;

    // These are for RTP Sender Reports, not currently implemented
    mLastSrTimeReal = tvZero();
    mLastSrTimeNtp = tvZero();
    mLastSrTimeRtp = 0;
}

/**
* @brief 
*
* @param seq
*/
void RtpSession::init( uint16_t seq )
{
    Debug( 3, "Initialising sequence" );
    // Most of these I don't think will ever be used but until RTP control is done not sure
    // so left them for now
    mBaseSeq = seq;
    mMaxSeq = seq;
    mBadSeq = OZ_RTP_SEQ_MOD + 1;  // so seq == mBadSeq is false
    mCycles = 0;
    //mSentPackets = 0;
    //mSentPrior = 0;
    mExpectedPrior = 0;
    // other initialization
    mJitter = 0;
    mTransit = 0;
}

/**
* @brief 
*
* @param seq
*
* @return 
*/
bool RtpSession::updateSeq( uint16_t seq )
{
    uint16_t uDelta = seq - mMaxSeq;

    // Source is not valid until MIN_SEQUENTIAL packets with
    // sequential sequence numbers have been received.
    Debug( 5, "Seq: %d", seq );

    if ( mProbation)
    {
        // packet is in sequence
        if ( seq == mMaxSeq + 1)
        {
            Debug( 3, "Sequence in probation %d, in sequence", mProbation );
            mProbation--;
            mMaxSeq = seq;
            if ( mProbation == 0 )
            {
                init( seq );
                mReceivedPackets++;
                return( true );
            }
        }
        else
        {
            Warning( "Sequence in probation %d, out of sequence", mProbation );
            mProbation = MIN_SEQUENTIAL - 1;
            mMaxSeq = seq;
            return( false );
        }
        return( true );
    }
    else if ( uDelta < MAX_DROPOUT )
    {
        if ( uDelta == 1 )
        {
            Debug( 3, "Packet in sequence, gap %d", uDelta );
        }
        else
        {
            Warning( "Packet in sequence, gap %d", uDelta );
        }

        // in order, with permissible gap
        if ( seq < mMaxSeq )
        {
            // Sequence number wrapped - count another 64K cycle.
            mCycles += OZ_RTP_SEQ_MOD;
        }
        mMaxSeq = seq;
    }
    else if ( uDelta <= OZ_RTP_SEQ_MOD - MAX_MISORDER )
    {
        Warning( "Packet out of sequence, gap %d", uDelta );
        // the sequence number made a very large jump
        if ( seq == mBadSeq )
        {
            Debug( 3, "Restarting sequence" );
            // Two sequential packets -- assume that the other side
            // restarted without telling us so just re-sync
            // (i.e., pretend this was the first packet).
            init( seq );
        }
        else
        {
            mBadSeq = (seq + 1) & (OZ_RTP_SEQ_MOD-1);
            return( false );
        }
    }
    else
    {
        Warning( "Packet duplicate or reordered, gap %d", uDelta );
        // duplicate or reordered packet
        return( false );
    }
    mReceivedPackets++;
    return( uDelta==1?true:false );
}

/**
* @brief 
*
* @param header
*/
void RtpSession::updateJitter( const RtpDataHeader *header )
{
    if ( mRtpFactor > 0 )
    {
        Debug( 5, "Delta rtp = %.6f", tvDiffSec( mBaseTimeReal ) );
        uint32_t localTimeRtp = mBaseTimeRtp + uint32_t( tvDiffSec( mBaseTimeReal ) * mRtpFactor );
        Debug( 5, "Local RTP time = %x", localTimeRtp );
        Debug( 5, "Packet RTP time = %x", ntohl(header->timestampN) );
        uint32_t packetTransit = localTimeRtp - ntohl(header->timestampN);
        Debug( 5, "Packet transit RTP time = %x", packetTransit );

        if ( mTransit > 0 )
        {
            // Jitter
            int d = packetTransit - mTransit;
            Debug( 5, "Jitter D = %d", d );
            if ( d < 0 )
                d = -d;
            //mJitter += (1./16.) * ((double)d - mJitter);
            mJitter += d - ((mJitter + 8) >> 4);
        }
        mTransit = packetTransit;
    }
    else
    {
        mJitter = 0;
    }
    Debug( 5, "RTP Jitter: %d", mJitter );
}

/**
* @brief 
*
* @param ntpTimeSecs
* @param ntpTimeFrac
* @param rtpTime
*/
void RtpSession::updateRtcpData( uint32_t ntpTimeSecs, uint32_t ntpTimeFrac, uint32_t rtpTime )
{
    struct timeval ntpTime = tvMake( ntpTimeSecs, suseconds_t((USEC_PER_SEC*(ntpTimeFrac>>16))/(1<<16)) );

    Debug( 5, "ntpTime: %ld.%06ld, rtpTime: %x", ntpTime.tv_sec, ntpTime.tv_usec, rtpTime );
                                                     
    if ( mBaseTimeNtp.tv_sec == 0 )
    {
        mBaseTimeReal = tvNow();
        mBaseTimeNtp = ntpTime;
        mBaseTimeRtp = rtpTime;
    }
    else if ( !mRtpClock )
    {
        Debug( 5, "lastSrNtpTime: %ld.%06ld, rtpTime: %x", mLastSrTimeNtp.tv_sec, mLastSrTimeNtp.tv_usec, rtpTime );
        Debug( 5, "ntpTime: %ld.%06ld, rtpTime: %x", ntpTime.tv_sec, ntpTime.tv_usec, rtpTime );

        double diffNtpTime = tvDiffSec( mBaseTimeNtp, ntpTime );
        uint32_t diffRtpTime = rtpTime - mBaseTimeRtp;

        //Debug( 5, "Real-diff: %.6f", diffRealTime );
        Debug( 5, "NTP-diff: %.6f", diffNtpTime );
        Debug( 5, "RTP-diff: %d", diffRtpTime );

        mRtpFactor = (uint32_t)(diffRtpTime / diffNtpTime);

        Debug( 5, "RTPfactor: %d", mRtpFactor );
    }
    mLastSrTimeNtpSecs = ntpTimeSecs;
    mLastSrTimeNtpFrac = ntpTimeFrac;
    mLastSrTimeNtp = ntpTime;
    mLastSrTimeRtp = rtpTime;
}

/**
* @brief 
*/
void RtpSession::updateRtcpStats()
{
    uint32_t extendedMax = mCycles + mMaxSeq;
    mExpectedPackets = extendedMax - mBaseSeq + 1;

    Debug( 5, "Expected packets = %d", mExpectedPackets );

    // The number of packets lost is defined to be the number of packets
    // expected less the number of packets actually received:
    mLostPackets = mExpectedPackets - mReceivedPackets;
    Debug( 5, "Lost packets = %d", mLostPackets );

    uint32_t expectedInterval = mExpectedPackets - mExpectedPrior;
    Debug( 5, "Expected interval = %d", expectedInterval );
    mExpectedPrior = mExpectedPackets;
    uint32_t receivedInterval = mReceivedPackets - mReceivedPrior;
    Debug( 5, "Received interval = %d", receivedInterval );
    mReceivedPrior = mReceivedPackets;
    uint32_t lostInterval = expectedInterval - receivedInterval;
    Debug( 5, "Lost interval = %d", lostInterval );

    if ( expectedInterval == 0 || lostInterval <= 0 )
        mLostFraction = 0;
    else
        mLostFraction = (lostInterval << 8) / expectedInterval;
    Debug( 5, "Lost fraction = %d", mLostFraction );
}


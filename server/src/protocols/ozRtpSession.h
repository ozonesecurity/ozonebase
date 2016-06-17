/** @addtogroup Protocols */
/*@{*/


#ifndef ZM_RTP_SESSION_H
#define ZM_RTP_SESSION_H

#include "../libgen/libgenBuffer.h"
#include "../libgen/libgenThread.h"

#define ZM_RTP_VERSION          2
#define ZM_RTP_MAX_PACKET_SIZE  1450

struct RtpDataHeader;

///
/// Class container elements specific to an RTP session, shared amongst one or more 
/// RTP streams.
///
class RtpSession
{
public: 
    typedef enum { EMPTY, FILLING, READY } FrameState;

private:
    static const int ZM_RTP_SEQ_MOD = 1<<16;
    static const int MAX_DROPOUT = 3000;
    static const int MAX_MISORDER = 100;
    static const int MIN_SEQUENTIAL = 2;

private:
    /// Identity
    uint32_t mSsrc;
    std::string mCname;             ///< Canonical name, for SDES

    /// RTP/RTCP fields
    uint16_t mMaxSeq;               ///< highest seq. number seen
    uint32_t mCycles;               ///< shifted count of seq. number cycles
    uint32_t mBaseSeq;              ///< base seq number
    uint32_t mBadSeq;               ///< last 'bad' seq number + 1
    uint32_t mProbation;            ///< sequ. packets till source is valid
    uint32_t mReceivedPackets;      ///< packets received
    uint32_t mExpectedPrior;        ///< packet expected at last interval
    uint32_t mReceivedPrior;        ///< packet received at last interval
    uint32_t mTransit;              ///< relative trans time for prev pkt
    uint32_t mJitter;               ///< estimated jitter
    
    /// Ports/Channels
    std::string mLocalHost;
    int mLocalPortChans[2]; 
    std::string mRemoteHost;
    int mRemotePortChans[2]; 

    /// Time keys
    uint32_t mRtpClock;
    uint32_t mRtpFactor;
    struct timeval mBaseTimeReal;
    struct timeval mBaseTimeNtp;
    uint32_t mBaseTimeRtp;

    struct timeval mLastSrTimeReal;
    uint32_t mLastSrTimeNtpSecs;
    uint32_t mLastSrTimeNtpFrac;
    struct timeval mLastSrTimeNtp;
    uint32_t mLastSrTimeRtp;

    /// Stats, intermittently updated
    uint32_t mExpectedPackets;
    uint32_t mLostPackets;
    uint8_t  mLostFraction;

    int mFrameCount;

private:
    void init( uint16_t seq );

public:
    RtpSession( uint32_t ssrc, const std::string &localHost, int localPortBase, const std::string &remoteHost, int remotePortBase );

    bool updateSeq( uint16_t seq );
    void updateJitter( const RtpDataHeader *header );
    void updateRtcpData( uint32_t ntpTimeSecs, uint32_t ntpTimeFrac, uint32_t rtpTime );
    void updateRtcpStats();

    bool handlePacket( const unsigned char *packet, size_t packetLen );

    uint32_t ssrc() const
    {
        return( mSsrc );
    }

    bool sendFrame( ByteBuffer &buffer );

    const std::string &getCname() const
    {
        return( mCname );
    }

    const std::string &localHost() const
    {
        return( mLocalHost );
    }

    int localDataPort() const
    {
        return( mLocalPortChans[0] );
    }

    int localCtrlPort() const
    {
        return( mLocalPortChans[1] );
    }

    const std::string &remoteHost() const
    {
        return( mRemoteHost );
    }

    int remoteDataPort() const
    {
        return( mRemotePortChans[0] );
    }

    int remoteCtrlPort() const
    {
        return( mRemotePortChans[1] );
    }

    uint32_t maxSeq() const
    {
        return( mCycles + mMaxSeq );
    }

    uint32_t expectedPackets() const
    {
        return( mExpectedPackets );
    }
    
    uint32_t lostPackets() const
    {
        return( mLostPackets );
    }

    uint8_t lostFraction() const
    {
        return( mLostFraction );
    }

    uint32_t jitter() const
    {
        return( mJitter >> 4 );
    }

    uint32_t lastSrTimestamp() const
    {
        return( ((mLastSrTimeNtpSecs&0xffff)<<16)|(mLastSrTimeNtpFrac>>16) );
    }
};

#endif // ZM_RTP_SESSION_H


/*@}*/

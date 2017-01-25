/** @addtogroup Protocols */
/*@{*/


#ifndef OZ_RTSP_STREAM_H
#define OZ_RTSP_STREAM_H

//
// Class representing the RTSP server thread. RTSP is defined in http://www.ietf.org/rfc/rfc2326.txt
// and SDP can be found at http://www.ietf.org/rfc/rfc2327.txt
// The RTSP server is a forking server able to handle any number of concurrent RTSP and RTP sessions
//

#include "../base/ozStream.h"
#include "../libgen/libgenThread.h"
#include "../libgen/libgenComms.h"
#include "../libgen/libgenBuffer.h"
#include "../base/ozFfmpeg.h"

#include <deque>

class RtspConnection;
class RtspSession;
class RtpSession;
class RtpDataManager;
class RtpDataHeader;
class RtpCtrlManager;
class RtpCtrlHeader;

// Class representing an RTSP stream. There is one RTSP stream per media type so for
// audio/video there would be. They are identified by track id. This class is also responsible
// for transmitting the encoded H.264 packets
class RtspStream : public Stream, public Thread
{
CLASSID(RtmpStream);

public:
    typedef enum { TRANS_RTP } Transport;
    typedef enum { PROF_AVP } Profile;
    typedef enum { LOWTRANS_UDP, LOWTRANS_TCP } LowerTransport;
    typedef enum { DEL_UNICAST, DEL_MULTICAST } Delivery;

private:
    RtspSession *mRtspSession;      // Pointer to the current RTSP session

    int mTrackId;                   // Identifier

    RtspConnection *mRtspConnection;

    Transport mTransport;           // Type of transport selected for this stream
    Profile mProfile;               // Profile selected for this stream
    LowerTransport mLowerTransport; // Lower transport selected for this stream

    Delivery mDelivery;             // Method of stream delivery
    std::string mDestAddr;          // Destination hostname or IP
    int mDestPorts[2];              // If sending over UDP which ports at the client end (one for data one for control)
    int mSourcePorts[2];            // If sending over UDP which ports do the packets originate from
    bool mInterleaved;              // Flag to indicate if RTP is interleaved alongside RTSP on TCP connection
    int mChannels[2];               // If interleaved what are the channel identifiers in use
    int mTtl;                       // Specified time-to-live of packets, not currently used

    uint32_t mSsrc;                 // The synchronisation source identifier
    uint32_t mSsrc1;                // The synchronisation source identifier

    std::string mHttpSession;       ///< Only for RTSP over HTTP sessions

    AVFormatContext *mFormatContext;// The ffmpeg library format context, see libavformat

    unsigned long mRtpTime;         // Time according to RTP

    RtpSession *mRtpSession;        // Pointer to associated RTP session
    RtpDataManager *mRtpData;       // Pointer to associated RTP data manager
    RtpCtrlManager *mRtpCtrl;       // Pointer to associated RTP control manager

public:
    RtspStream( RtspSession *session, int trackId, RtspConnection *connection, Encoder *encoder, const std::string &transport, const std::string &profile, const std::string &lowerTransport, const StringTokenList::TokenList &transportParms );
    ~RtspStream();

public:
    bool isValidSsrc( uint32_t ssrc );
    bool updateSsrc( uint32_t ssrc )
    {
        if ( mSsrc1 )
        {
            if ( ssrc != mSsrc1 )
                return( false );
        }
        else if ( ssrc )
            mSsrc1 = ssrc;
        return( true );
    }

    int trackId() const
    {
        return( mTrackId );
    }

    Transport transport() const
    {
        return( mTransport );
    }
    Profile profile() const
    {
        return( mProfile );
    }
    LowerTransport lowerTransport() const
    {
        return( mLowerTransport );
    }
    Delivery delivery() const
    {
        return( mDelivery );
    }
    bool interleaved() const
    {
        return( mInterleaved );
    }
    int destPort( int index ) const
    {
        return( (index>=0 && index<=1)?mDestPorts[index]:-1 );
    }
    int sourcePort( int index ) const
    {
        return( (index>=0 && index<=1)?mSourcePorts[index]:-1 );
    }
    int channel( int index ) const
    {
        return( (index>=0 && index<=1)?mChannels[index]:-1 );
    }
    uint32_t ssrc() const
    {
        return( mSsrc );
    }
    uint32_t ssrc1() const
    {
        return( mSsrc1 );
    }

    AVFormatContext *getFormatContext()
    {
        return( mFormatContext );
    }

    void packetiseFrame( const FramePtr &frame );
 
protected:
    int run();
};

#endif // OZ_RTSP_STREAM_H


/*@}*/

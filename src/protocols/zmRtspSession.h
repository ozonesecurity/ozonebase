#ifndef ZM_RTSP_SESSION_H
#define ZM_RTSP_SESSION_H

#include "zmRtspConnection.h"

#include <map>
#include <set>

class RtspController;
class RtspStream;
class Encoder;

/// Class representing one RTSP session from start to finish. This may or may not correlate to one TCP connection.
class RtspSession
{
public:
    typedef enum { INIT, READY, PLAYING } State;

private:
    typedef std::map<int,RtspStream *> RtspStreams;

    typedef std::set<int>           PortSet;
    typedef std::set<uint32_t>      SsrcSet;

private:
    State           mState;             ///< RTSP state (from RFC)

    uint32_t        mSession;           ///< Session identifier
    RtspConnection  *mConnection;
    Encoder         *mEncoder;
         
    RtspStreams     mRtspStreams;       ///< Streams represented in this session, will normally be 1 or 2

public:
    RtspSession( int session, RtspConnection *connection, Encoder *encoder );
    ~RtspSession();

public:
    bool recvRequest( const std::string &requestType, const std::string &requestUrl, const Connection::Headers &requestHeaders, Connection::Headers &responseHeaders );
    RtspStream *newStream( int trackId, Encoder *encoder, const std::string &transportSpec );

    State state() const { return( mState ); }
    State state( State newState );

    uint32_t session() const { return( mSession ); }

    int requestPorts();
    void releasePorts( int port );

    uint32_t requestSsrc();
    void releaseSsrc( uint32_t ssrc );
};

#endif // ZM_RTSP_SESSION_H

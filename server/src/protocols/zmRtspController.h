#ifndef ZM_RTSP_CONTROLLER_H
#define ZM_RTSP_CONTROLLER_H

#include "../zmController.h"

#include <map>
#include <set>

class RtspConnection;
class RtspSession;
class Encoder;

// Primary RTSP protocol controller class. Manages all protocol specific elements
class RtspController : public Controller
{
CLASSID(RtspController);

private:
    typedef std::map<uint32_t,RtspSession *> RtspSessions;

private:
    typedef std::set<int>           PortSet;
    typedef std::set<uint32_t>      SsrcSet;

public:
    typedef std::pair<int,int>      PortRange;

private:
    PortRange   mDataPortRange;             // Port range available to be assigned to pool
    SsrcSet     mSsrcs;                     // SSRC pool, to prevented conflicts and duplicates
    PortSet     mAssignedPorts;             // UDP port pool, RTSP sessions request and release ports via this object

private:
    RtspSessions mRtspSessions;             // Set of RTSP session indexed by SSRC

public:
    RtspController( const std::string &name, uint32_t tcpPort, const PortRange &udpPortRange );
    ~RtspController();

public:
    Connection *newConnection( TcpInetSocket *socket );
    RtspSession *getSession( uint32_t session );
    RtspSession *newSession( RtspConnection *connection, Encoder *encoder );

    int requestPorts();
    void releasePorts( int port );

    uint32_t requestSsrc();
    void releaseSsrc( uint32_t ssrc );
};

#endif // ZM_RTSP_CONTROLLER_H

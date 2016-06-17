/** @addtogroup Protocols */
/*@{*/


#ifndef ZM_RTMP_CONTROLLER_H
#define ZM_RTMP_CONTROLLER_H

#include "../base/zmController.h"

#include <map>

class RtmpConnection;
class RtmpSession;

// Primary RTMP protocol controller class. Manages all protocol specific elements
class RtmpController : public Controller
{
CLASSID(RtmpController);

private:
    typedef std::map<uint32_t,RtmpSession *> RtmpSessions;

private:
    RtmpSessions mRtmpSessions;             // Set of RTMP session indexed by chunk stream id

public:
    RtmpController( const std::string &name, uint32_t port ) :
        Controller( cClass(), name, "RTMP", port )
    {
    }
    ~RtmpController()
    {
    }

public:
    Connection *newConnection( TcpInetSocket *socket );
    RtmpSession *getSession( uint32_t session );
    RtmpSession *newSession( uint32_t session, RtmpConnection *connection );
    void deleteSession( uint32_t session );
};

#endif // ZM_RTMP_CONTROLLER_H


/*@}*/

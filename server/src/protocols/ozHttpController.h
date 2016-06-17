/** @addtogroup Protocols */
/*@{*/


#ifndef OZ_HTTP_CONTROLLER_H
#define OZ_HTTP_CONTROLLER_H

#include "../base/ozController.h"

#include <map>

class HttpSession;

// Primary HTTP protocol controller class. Manages all protocol specific elements
class HttpController : public Controller
{
CLASSID(HttpController);

private:
    typedef std::map<uint32_t,HttpSession *> HttpSessions;

private:
    HttpSessions    mHttpSessions;

public:
    HttpController( const std::string &name, int port ) :
        Controller( cClass(), name, "HTTP", port )
    {
    }
    ~HttpController()
    {
    }

public:
    Connection *newConnection( TcpInetSocket *socket );
    HttpSession *getSession( uint32_t session );
    HttpSession *newSession( uint32_t session );
    void deleteSession( uint32_t session );
};

#endif // OZ_HTTP_CONTROLLER_H


/*@}*/

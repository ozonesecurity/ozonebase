/** @addtogroup Utilities */
/*@{*/
#ifndef OZ_LISTENER_H
#define OZ_LISTENER_H

#include "../libgen/libgenComms.h"
#include "../libgen/libgenThread.h"

#include <map>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

class Controller;
class Connection;

///
/// Main socket listener class. Forks off protocol specific requests as required
///
class Listener : public Thread
{
private:
    typedef std::map<uint32_t,Controller *>         ControllerMap;
    typedef std::map<TcpInetSocket *,Connection *>  Connections;

private:
    std::string     mInterface;         ///< Which network interface to listen on, usually either 'any' or specific IP, e.g. '127.0.0.1'
    ControllerMap   mControllers;       ///< Map of registered protocol controllers indexed by protocol
    Connections     mConnections;       ///< Map of connections indexed by socket address

public:
    Listener( const std::string &interface="any" );
    ~Listener();

    void addController( Controller *controller );       ///< Add a controller to permit new connections using specific protocol
    void removeController( Controller *controller );    ///< Remove a controller, prevents new connections using specific protocol

private:
    int run();                                          ///< Execute listener thread
};

#endif // OZ_LISTENER_H
/*@}*/

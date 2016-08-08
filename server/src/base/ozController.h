/** @addtogroup Utilities */
/*@{*/
#ifndef OZ_CONTROLLER_H
#define OZ_CONTROLLER_H

#include "ozFeedConsumer.h"
#include <map>

class Connection;
class TcpInetSocket;

///
/// Abstract base class for protocol controllers used by protocol specific connections. Controllers manage protocol
/// specific settings and manage other objects.
///
class Controller : public VideoConsumer
{
private:
    typedef std::map<std::string,std::string>       ApplicationClassMap;
    typedef std::map<std::string,FeedProvider *>    ApplicationInstanceMap;

protected:
    std::string             mProto;                     ///< Protocol identification string
    uint32_t                mPort;                      ///< Key port for listening on
    ApplicationClassMap     mApplicationClasses;        ///< Set of application names to provider classes
    ApplicationInstanceMap  mApplicationInstances;      ///< Set of application names/paths to provider instances

public:
    static std::string makePath( const std::string &streamName, const std::string streamSource )
    {
        return( streamName+"/"+streamSource );
    }

protected:
    Controller( const std::string &tag, const std::string &id, const std::string &proto, uint32_t port ) :
        VideoConsumer( tag, id, 5 ),
        mProto( proto ),
        mPort( port )
    {
    }

public:
    virtual ~Controller()
    {
    }

    virtual Connection *newConnection( TcpInetSocket *socket )=0;   ///< Create a new connection
    const std::string &proto() const { return( mProto ); }          ///< Return the protocol string
    const char *cproto() const { return( mProto.c_str() ); } 
    uint32_t port() const { return( mPort ); }

    void addStream( const std::string &streamName, const std::string &streamClass );
    void addStream( const std::string &streamPath, FeedProvider &streamProvider );
    void removeStream( const std::string &streamTag );
    bool verifyStreamName( const std::string &streamName );
    FeedProvider *findStream( const std::string &streamName, const std::string &streamSource );
};

#endif // OZ_CONTROLLER_H
/*@}*/

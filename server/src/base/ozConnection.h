#ifndef OZ_CONNECTION_H
#define OZ_CONNECTION_H

#include "../libgen/libgenComms.h"
#include "../libgen/libgenBuffer.h"

#include <map>

class Controller;

///
/// Class representing a single TCP connection to the server. This does not correlate with a specific
/// session but is used primarily to maintain state and content between requests, especially partial ones,
/// and manage access to the socket.
/// This is an abstract base class and must be specialised for the protocols actually used
///
class Connection
{
public:
    typedef std::map<const std::string, const std::string> Headers;
    typedef enum { TEXT, BINARY } ConnectionType;

protected:
    Controller      *mController;       ///< Pointer to controller that generated this connection
    TcpInetSocket   *mSocket;           ///< Pointer to the socket used for this connection
    ConnectionType  mConnectionType;    ///< Type of connection, text or binary

public:
    Connection( Controller *controller, TcpInetSocket *socket, ConnectionType connectionType ) :
        mController( controller ),
        mSocket( socket ),
        mConnectionType( connectionType )
    {
    }
    virtual ~Connection()
    {
    }

public:
    bool isText() const
    {
        return( mConnectionType == TEXT );
    }
    bool isBinary() const
    {
        return( mConnectionType == BINARY );
    }
    virtual bool recvRequest( std::string &request )
    {
        if ( !isText() )
            Warning( "Calling text receiver for binary connection" );
        Panic( "No text receiver defined" );
        return( false );
    }
    virtual bool recvRequest( ByteBuffer &request )
    {
        if ( !isBinary() )
            Warning( "Calling binary receiver for text connection" );
        Panic( "No binary receiver defined" );
        return( false );
    }

public:
    Controller *controller() { return( mController ); }
    TcpInetSocket *socket() { return( mSocket ); }
    const SockAddrInet *getRemoteAddr() const { return( mSocket->getRemoteAddr() ); }
    const SockAddrInet *getLocalAddr() const { return( mSocket->getLocalAddr() ); }
};

/// Class representing text based protocol, e.g. HTTP
class TextConnection : public Connection
{
public:
    typedef std::map<const std::string, const std::string> Headers;

protected:
    std::string mRequest;       ///< Only used when partial request are received, saves previous content

public:
    TextConnection( Controller *controller, TcpInetSocket *socket ) :
        Connection( controller, socket, TEXT )
    {
    }

public:
    virtual bool recvRequest( std::string &request )=0;
};

/// Class representing binary protocol, e.g. RTMP
class BinaryConnection : public Connection
{
protected:
    ByteBuffer mRequest;        ///< Only used when partial request are received, saves previous content

public:
    BinaryConnection( Controller *controller, TcpInetSocket *socket ) :
        Connection( controller, socket, BINARY )
    {
    }

public:
    virtual bool recvRequest( ByteBuffer &request )=0;
};

#endif // OZ_CONNECTION_H

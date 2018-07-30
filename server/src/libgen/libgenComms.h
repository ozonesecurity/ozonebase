#ifndef LIBGEN_COMMS_H
#define LIBGEN_COMMS_H

#include "libgenDebug.h"
#include "libgenException.h"
#include "oz.h"

#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/uio.h>

#include <set>
#include <vector>

class CommsException : public Exception
{
public:
    CommsException( const std::string &message ) : Exception( message )
    {
    }
};

/// Base class for all the Socket and Pipe classes
class CommsBase
{
protected:
    const int   &mRd;
    const int   &mWd;

protected:
    CommsBase( int &rd, int &wd ) : mRd( rd ), mWd( wd )
    {
    }
    virtual ~CommsBase()
    {
    }

public:
    virtual bool close()=0;
    virtual bool isOpen() const=0;
    virtual bool isClosed() const=0;
    virtual bool setBlocking( bool blocking )=0;

public:
    int getReadDesc() const
    {
        return( mRd );
    }
    int getWriteDesc() const
    {
        return( mWd );
    }
    int getMaxDesc() const
    {
        return( mRd>mWd?mRd:mWd );
    }

    virtual int read( void *msg, int len )
    {
        ssize_t nBytes = ::read( mRd, msg, len );
        if ( nBytes < 0 )
            Debug( 1, "Read of %d bytes max on rd %d failed: %s", len, mRd, strerror(errno) );
        return( nBytes );
    }
    virtual int write( const void *msg, int len )
    {
        ssize_t nBytes = ::write( mWd, msg, len );
        if ( nBytes < 0 )
            Debug( 1, "Write of %d bytes on wd %d failed: %s", len, mWd, strerror(errno) );
        return( nBytes );
    }
    virtual int readV( const struct iovec *iov, int iovcnt )
    {
        int nBytes = ::readv( mRd, iov, iovcnt );
        if ( nBytes < 0 )
            Debug( 1, "Readv of %d buffers max on rd %d failed: %s", iovcnt, mRd, strerror(errno) );
        return( nBytes );
    }
    virtual int writeV( const struct iovec *iov, int iovcnt )
    {
        ssize_t nBytes = ::writev( mWd, iov, iovcnt );
        if ( nBytes < 0 )
            Debug( 1, "Writev of %d buffers on wd %d failed: %s", iovcnt, mWd, strerror(errno) );
        return( nBytes );
    }
    virtual int readV( int iovcnt, /* const void *msg1, int len1, */ ... );
    virtual int writeV( int iovcnt, /* const void *msg1, int len1, */ ... );
};

/// Class representing a unix pipe
class Pipe : public CommsBase
{
protected:
    int mFd[2];

public:
    Pipe() : CommsBase( mFd[0], mFd[1] )
    {
        mFd[0] = -1;
        mFd[1] = -1;
    }
    ~Pipe()
    {
        close();
    }

public:
    bool open();
    bool close();

    bool isOpen() const
    {
        return( mFd[0] != -1 && mFd[1] != -1 );
    }
    int getReadDesc() const
    {
        return( mFd[0] );
    }
    int getWriteDesc() const
    {
        return( mFd[1] );
    }

    bool setBlocking( bool blocking );
};

/// Class representing a general socket address
class SockAddr
{
private:
    const struct sockaddr *mAddr;

public:
    SockAddr( const struct sockaddr *addr );
    virtual ~SockAddr()
    {
    }

    static SockAddr *newSockAddr( const struct sockaddr &addr, socklen_t len );
    static SockAddr *newSockAddr( const SockAddr *addr );

    int getDomain() const
    {
        return( mAddr?mAddr->sa_family:AF_UNSPEC );
    }

    const struct sockaddr *getAddr() const
    {
        return( mAddr );
    }
    virtual socklen_t getAddrSize() const=0;
    virtual struct sockaddr *getTempAddr() const=0;
};

/// Class representing an internet socket address
class SockAddrInet : public SockAddr
{
private:
    struct sockaddr_in  mAddrIn;
    struct sockaddr_in  mTempAddrIn;

    mutable std::string mHost;  // Temporary storage for hostname resolution

public:
    SockAddrInet();
    SockAddrInet( const SockAddrInet &addr ) : SockAddr( (const struct sockaddr *)&mAddrIn ), mAddrIn( addr.mAddrIn )
    {
    }
    SockAddrInet( const struct sockaddr_in *addr ) : SockAddr( (const struct sockaddr *)&mAddrIn ), mAddrIn( *addr )
    {
    }

    bool resolve( const char *host, const char *serv, const char *proto );
    bool resolve( const char *host, int port, const char *proto );
    bool resolve( const char *serv, const char *proto );
    bool resolve( int port, const char *proto );

    socklen_t getAddrSize() const
    {
        return( sizeof(mAddrIn) );
    }
    const std::string &host() const
    {
        mHost = inet_ntoa( mAddrIn.sin_addr );
        return( mHost );
    }
    int port() const
    {
        return( mAddrIn.sin_port );
    }
    struct sockaddr *getTempAddr() const
    {
        return( (sockaddr *)&mTempAddrIn );
    }

public:
    static socklen_t addrSize()
    {
        return( sizeof(sockaddr_in) );
    }
};

/// Class representing a Unix socket address
class SockAddrUnix : public SockAddr
{
private:
    struct sockaddr_un  mAddrUn;
    struct sockaddr_un  mTempAddrUn;

    mutable std::string mPath;  // Temporary storage for path

public:
    SockAddrUnix();
    SockAddrUnix( const SockAddrUnix &addr ) : SockAddr( (const struct sockaddr *)&mAddrUn ), mAddrUn( addr.mAddrUn )
    {
    }
    SockAddrUnix( const struct sockaddr_un *addr ) : SockAddr( (const struct sockaddr *)&mAddrUn ), mAddrUn( *addr )
    {
    }

    bool resolve( const char *path, const char *proto );

    socklen_t getAddrSize() const
    {
        return( sizeof(mAddrUn) );
    }
    const std::string &path() const
    {
        mPath = mAddrUn.sun_path;
        return( mPath );
    }
    struct sockaddr *getTempAddr() const
    {
        return( (sockaddr *)&mTempAddrUn );
    }

public:
    static socklen_t addrSize()
    {
        return( sizeof(sockaddr_un) );
    }
};

/// Abstract base class representing a socket protocol
class SocketProtocol
{
public:
    virtual ~SocketProtocol() {}

public:
    virtual int getType() const=0;
    virtual const char *getProtocol() const=0;
};

/// Class representing the TCP socket protocol
class TcpSocketProtocol : public SocketProtocol
{
public:
    int getType() const
    {
        return( SOCK_STREAM );
    }
    const char *getProtocol() const
    {
        return( "tcp" );
    }
};

/// Class representing the UDP socket protocol
class UdpSocketProtocol : public SocketProtocol
{
public:
    int getType() const
    {
        return( SOCK_DGRAM );
    }
    const char *getProtocol() const
    {
        return( "udp" );
    }

public:
    int sendto( int sd, const void *msg, int len, const SockAddr *addr=0 ) const
    {
        ssize_t nBytes = ::sendto( sd, msg, len, 0, addr?addr->getAddr():NULL, addr?addr->getAddrSize():0 );
        if ( nBytes < 0 )
            Debug( 1, "Sendto of %d bytes on sd %d failed: %s", len, sd, strerror(errno) );
        return( nBytes );
    }
    int recvfrom( int sd, void *msg, int len, SockAddr *addr=0 ) const
    {
        ssize_t nBytes = 0;
        if ( addr )
        {
            struct sockaddr sockAddr;
            socklen_t sockLen;
            nBytes = ::recvfrom( sd, msg, len, 0, &sockAddr, &sockLen );
            if ( nBytes < 0 )
            {
                Debug( 1, "Recvfrom of %d bytes max on sd %d (with address) failed: %s", len, sd, strerror(errno) );
            }
            else if ( sockLen )
            {
                addr = SockAddr::newSockAddr( sockAddr, sockLen );
            }
        }   
        else
        {
            nBytes = ::recvfrom( sd, msg, len, 0, NULL, 0 );
            if ( nBytes < 0 )
                Debug( 1, "Recvfrom of %d bytes max on sd %d (no address) failed: %s", len, sd, strerror(errno) );
        }
        return( nBytes );
    }
};

/// Class representing a general Unix socket
class Socket : public CommsBase
{
protected:
    typedef enum { CLOSED, DISCONNECTED, LISTENING, CONNECTED } State;

protected:
    int mSd;
    State mState;
    SockAddr *mLocalAddr;
    SockAddr *mRemoteAddr;

protected:
    Socket() :
        CommsBase( mSd, mSd ),
        mSd( -1 ),
        mState( CLOSED ),
        mLocalAddr( 0 ),
        mRemoteAddr( 0 )
    {
    }
    Socket( const Socket &socket, int newSd ) :
        CommsBase( mSd, mSd ),
        mSd( newSd ),
        mState( CONNECTED ),
        mLocalAddr( 0 ),
        mRemoteAddr( 0 )
    {
        if ( socket.mLocalAddr )
            mLocalAddr = SockAddr::newSockAddr( socket.mLocalAddr );
        if ( socket.mRemoteAddr )
            mRemoteAddr = SockAddr::newSockAddr( socket.mRemoteAddr );
    }
    virtual ~Socket()
    {
        close();
        delete mLocalAddr;
        delete mRemoteAddr;
    }

public:
    bool isOpen() const
    {
        return( !isClosed() );
    }
    bool isClosed() const
    {
        return( mState == CLOSED );
    }
    bool isDisconnected() const
    {
        return( mState == DISCONNECTED );
    }
    bool isConnected() const
    {
        return( mState == CONNECTED );
    }
    virtual bool close();

protected:
    bool isListening() const
    {
        return( mState == LISTENING );
    }

protected:
    bool socket();
    bool bind();

protected:
    bool connect();
    // Virtual only to allow protection status override in server classes
    bool listen();
    bool accept();
    bool accept( int & );

public:
    virtual int send( const void *msg, int len ) const
    {
        ssize_t nBytes = ::send( mSd, msg, len, 0 );
        if ( nBytes < 0 )
            Debug( 1, "Send of %d bytes on sd %d failed: %s", len, mSd, strerror(errno) );
        return( nBytes );
    }
    virtual int recv( void *msg, int len ) const
    {
        ssize_t nBytes = ::recv( mSd, msg, len, 0 );
        if ( nBytes < 0 )
            Debug( 1, "Recv of %d bytes max on sd %d failed: %s", len, mSd, strerror(errno) );
        return( nBytes );
    }
    virtual int send( const std::string &msg ) const
    {
        ssize_t nBytes = ::send( mSd, msg.data(), msg.size(), 0 );
        if ( nBytes < 0 )
            Debug( 1, "Send of string '%s' (%zd bytes) on sd %d failed: %s", msg.c_str(), msg.size(), mSd, strerror(errno) );
        return( nBytes );
    }
    virtual int recv( std::string &msg ) const
    {
        char buffer[msg.capacity()];
        int nBytes = 0;
        if ( (nBytes = ::recv( mSd, buffer, sizeof(buffer), 0 )) < 0 )
        {
            Debug( 1, "Recv of %zd bytes max to string on sd %d failed: %s", sizeof(buffer), mSd, strerror(errno) );
            return( nBytes );
        }
        buffer[nBytes] = '\0';
        msg = buffer;
        return( nBytes );
    }
    virtual int recv( std::string &msg, size_t maxLen ) const
    {
        char buffer[maxLen];
        int nBytes = 0;
        if ( (nBytes = ::recv( mSd, buffer, sizeof(buffer), 0 )) < 0 )
        {
            Debug( 1, "Recv of %zd bytes max to string on sd %d failed: %s", maxLen, mSd, strerror(errno) );
            return( nBytes );
        }
        buffer[nBytes] = '\0';
        msg = buffer;
        return( nBytes );
    }
    virtual int bytesToRead() const;

    int getDesc() const
    {
        return( mSd );
    }
    //virtual bool isOpen() const
    //{
        //return( mSd != -1 );
    //}

    virtual int getDomain() const=0;
    virtual int getType() const=0;
    virtual const char *getProtocol() const=0;

    const SockAddr *getLocalAddr() const
    {
        return( mLocalAddr );
    }
    const SockAddr *getRemoteAddr() const
    {
        return( mRemoteAddr );
    }
    virtual socklen_t getAddrSize() const=0;

    bool getBlocking( bool &blocking );
    bool setBlocking( bool blocking );

    bool getSendBufferSize( int & ) const;
    bool getRecvBufferSize( int & ) const;

    bool setSendBufferSize( int );
    bool setRecvBufferSize( int );

    bool getRouting( bool & ) const;
    bool setRouting( bool );

    bool getNoDelay( bool & ) const;
    bool setNoDelay( bool );
};

/// Class representing a general socket using internet addressing
class InetSocket: public Socket
{
public:
    InetSocket()
    {
    }
    InetSocket( const InetSocket &socket, int newSd ) : Socket( socket, newSd )
    {
    }

public:
    int getDomain() const
    {
        return( AF_INET );
    }
    virtual socklen_t getAddrSize() const
    {
        return( SockAddrInet::addrSize() );
    }
    const SockAddrInet *getLocalAddr() const
    {
        return( dynamic_cast<SockAddrInet *>( mLocalAddr ) );
    }
    const SockAddrInet *getRemoteAddr() const
    {
        return( dynamic_cast<SockAddrInet *>( mRemoteAddr ) );
    }

protected:
    SockAddrInet *newAddr( SockAddr *&addrAlias )
    {
        if ( addrAlias )
            delete addrAlias;
        SockAddrInet *newAddr = new SockAddrInet;
        addrAlias = newAddr;
        return( newAddr );
    }

    bool resolveLocal( const char *host, const char *serv, const char *proto )
    {
        SockAddrInet *tempAddr = newAddr( mLocalAddr );
        return( tempAddr->resolve( host, serv, proto ) );
    }
    bool resolveLocal( const char *host, int port, const char *proto )
    {
        SockAddrInet *tempAddr = newAddr( mLocalAddr );
        return( tempAddr->resolve( host, port, proto ) );
    }
    bool resolveLocal( const char *serv, const char *proto )
    {
        SockAddrInet *tempAddr = newAddr( mLocalAddr );
        return( tempAddr->resolve( serv, proto ) );
    }
    bool resolveLocal( int port, const char *proto )
    {
        SockAddrInet *tempAddr = newAddr( mLocalAddr );
        return( tempAddr->resolve( port, proto ) );
    }

    bool resolveRemote( const char *host, const char *serv, const char *proto )
    {
        SockAddrInet *tempAddr = newAddr( mRemoteAddr );
        return( tempAddr->resolve( host, serv, proto ) );
    }
    bool resolveRemote( const char *host, int port, const char *proto )
    {
        SockAddrInet *tempAddr = newAddr( mRemoteAddr );
        return( tempAddr->resolve( host, port, proto ) );
    }

protected:
    bool bind( const SockAddrInet &addr )
    {
        if ( mLocalAddr )
            delete mLocalAddr;
        mLocalAddr = new SockAddrInet( addr );
        return( Socket::bind() );
    }
    bool bind( const char *host, const char *serv ) 
    {
        if ( !resolveLocal( host, serv, getProtocol() ) )
            return( false );
        return( Socket::bind() );
    }
    bool bind( const char *host, int port )
    {
        if ( !resolveLocal( host, port, getProtocol() ) )
            return( false );
        return( Socket::bind() );
    }
    bool bind( const char *serv )
    {
        if ( !resolveLocal( serv, getProtocol() ) )
            return( false );
        return( Socket::bind() );
    }
    bool bind( int port )
    {
        if ( !resolveLocal( port, getProtocol() ) )
            return( false );
        return( Socket::bind() );
    }

    bool connect( const SockAddrInet &addr )
    {
        if ( mRemoteAddr )
            delete mRemoteAddr;
        mRemoteAddr = new SockAddrInet( addr );
        return( Socket::connect() );
    }
    bool connect( const char *host, const char *serv )
    {
        if ( !resolveRemote( host, serv, getProtocol() ) )
            return( false );
        return( Socket::connect() );
    }
    bool connect( const char *host, int port )
    {
        if ( !resolveRemote( host, port, getProtocol() ) )
            return( false );
        return( Socket::connect() );
    }
};

/// Class representing a general socket using Unix style addressing
class UnixSocket : public Socket
{
public:
    UnixSocket()
    {
    }
    UnixSocket( const UnixSocket &socket, int newSd ) : Socket( socket, newSd )
    {
    }

public:
    int getDomain() const
    {
        return( AF_UNIX );
    }
    virtual socklen_t getAddrSize() const
    {
        return( SockAddrUnix::addrSize() );
    }
    const SockAddrUnix *getLocalAddr() const
    {
        return( dynamic_cast<SockAddrUnix *>( mLocalAddr ) );
    }
    const SockAddrUnix *getRemoteAddr() const
    {
        return( dynamic_cast<SockAddrUnix *>( mRemoteAddr ) );
    }

protected:
    SockAddrUnix *newAddr( SockAddr *&addrAlias )
    {
        if ( addrAlias )
            delete addrAlias;
        SockAddrUnix *newAddr = new SockAddrUnix;
        addrAlias = newAddr;
        return( newAddr );
    }

    bool resolveLocal( const char *path, const char *proto )
    {
        SockAddrUnix *tempAddr = newAddr( mLocalAddr );
        return( tempAddr->resolve( path, proto ) );
    }

    bool resolveRemote( const char *path, const char *proto )
    {
        SockAddrUnix *tempAddr = newAddr( mRemoteAddr );
        return( tempAddr->resolve( path, proto ) );
    }

protected:
    bool bind( const SockAddrUnix &addr )
    {
        if ( mLocalAddr )
            delete mLocalAddr;
        mLocalAddr = new SockAddrUnix( addr );
        return( Socket::bind() );
    }
    bool bind( const char *path )
    {
        if ( !UnixSocket::resolveLocal( path, getProtocol() ) )
            return( false );
        return( Socket::bind() );
    }

    bool connect( const SockAddrUnix &addr )
    {
        if ( mRemoteAddr )
            delete mRemoteAddr;
        mRemoteAddr = new SockAddrUnix( addr );
        return( Socket::connect() );
    }
    bool connect( const char *path )
    {
        if ( !UnixSocket::resolveRemote( path, getProtocol() ) )
            return( false );
        return( Socket::connect() );
    }
};

/// Class representing a UDP internet socket
class UdpInetSocket : public InetSocket, public UdpSocketProtocol
{
public:
    UdpInetSocket() 
    {
    }

public:
    int getType() const
    {
        return( UdpSocketProtocol::getType() );
    }
    const char *getProtocol() const
    {
        return( UdpSocketProtocol::getProtocol() );
    }

public:
    int sendto( const void *msg, int len, const SockAddr *addr=0 ) const
    {
        return( UdpSocketProtocol::sendto( mSd, msg, len, addr ) );
    }
    int recvfrom( void *msg, int len, SockAddr *addr=0 ) const
    {
        return( UdpSocketProtocol::recvfrom( mSd, msg, len, addr ) );
    }
};

/// Class representing a UDP Unix socket
class UdpUnixSocket : public UnixSocket, public UdpSocketProtocol
{
public:
    UdpUnixSocket() 
    {
    }

public:
    int getType() const
    {
        return( UdpSocketProtocol::getType() );
    }
    const char *getProtocol() const
    {
        return( UdpSocketProtocol::getProtocol() );
    }

public:
    bool bind( const char *path )
    {
        return( UnixSocket::bind( path ) );
    }

    bool connect( const char *path )
    {
        return( UnixSocket::connect( path ) );
    }
};

/// Class representing a UDP internet client socket
class UdpInetClient : public UdpInetSocket
{
protected:
    bool bind( const SockAddrInet &addr ) 
    {
        return( UdpInetSocket::bind( addr ) );
    }
    bool bind( const char *host, const char *serv )
    {
        return( UdpInetSocket::bind( host, serv ) );
    }
    bool bind( const char *host, int port )
    {
        return( UdpInetSocket::bind( host, port ) );
    }
    bool bind( const char *serv )
    {
        return( UdpInetSocket::bind( serv ) );
    }
    bool bind( int port )
    {
        return( UdpInetSocket::bind( port ) );
    }

public:
    bool connect( const SockAddrInet &addr ) 
    {
        return( UdpInetSocket::connect( addr ) );
    }
    bool connect( const char *host, const char *serv )
    {
        return( UdpInetSocket::connect( host, serv ) );
    }
    bool connect( const char *host, int port )
    {
        return( UdpInetSocket::connect( host, port ) );
    }
};

/// Class representing a UDP Unix client socket
class UdpUnixClient : public UdpUnixSocket
{
public:
    bool bind( const char *path )
    {
        return( UdpUnixSocket::bind( path ) );
    }

public:
    bool connect( const char *path )
    {
        return( UdpUnixSocket::connect( path) );
    }
};

/// Class representing a UDP internet server socket
class UdpInetServer : public UdpInetSocket
{
public:
    bool bind( const SockAddrInet &addr ) 
    {
        return( UdpInetSocket::bind( addr ) );
    }
    bool bind( const char *host, const char *serv )
    {
        return( UdpInetSocket::bind( host, serv ) );
    }
    bool bind( const char *host, int port )
    {
        return( UdpInetSocket::bind( host, port ) );
    }
    bool bind( const char *serv )
    {
        return( UdpInetSocket::bind( serv ) );
    }
    bool bind( int port )
    {
        return( UdpInetSocket::bind( port ) );
    }

public:
    bool connect( const SockAddrInet &addr ) 
    {
        return( UdpInetSocket::connect( addr ) );
    }
    bool connect( const char *host, const char *serv )
    {
        return( UdpInetSocket::connect( host, serv ) );
    }
    bool connect( const char *host, int port )
    {
        return( UdpInetSocket::connect( host, port ) );
    }
};

/// Class representing a UDP Unix server socket
class UdpUnixServer : public UdpUnixSocket
{
public:
    bool bind( const char *path )
    {
        return( UdpUnixSocket::bind( path ) );
    }

protected:
    bool connect( const char *path )
    {
        return( UdpUnixSocket::connect( path ) );
    }
};

/// Class representing a TCP internet socket
class TcpInetSocket : public InetSocket, public TcpSocketProtocol
{
public:
    TcpInetSocket() 
    {
    }
    TcpInetSocket( const TcpInetSocket &socket, int newSd ) : InetSocket( socket, newSd )
    {
    }

public:
    int getType() const
    {
        return( TcpSocketProtocol::getType() );
    }
    const char *getProtocol() const
    {
        return( TcpSocketProtocol::getProtocol() );
    }
};

/// Class representing a TCP Unix socket
class TcpUnixSocket : public UnixSocket, public UdpSocketProtocol
{
public:
    TcpUnixSocket()
    {
    }
    TcpUnixSocket( const TcpUnixSocket &socket, int newSd ) : UnixSocket( socket, newSd )
    {
    }

public:
    int getType() const
    {
        return( UdpSocketProtocol::getType() );
    }
    const char *getProtocol() const
    {
        return( UdpSocketProtocol::getProtocol() );
    }
};

/// Class representing a TCP internet client socket
class TcpInetClient : public TcpInetSocket
{
public:
    bool connect( const char *host, const char *serv )
    {
        return( TcpInetSocket::connect( host, serv ) );
    }
    bool connect( const char *host, int port )
    {
        return( TcpInetSocket::connect( host, port ) );
    }
};

/// Class representing a TCP Unix client socket
class TcpUnixClient : public TcpUnixSocket
{
public:
    bool connect( const char *path )
    {
        return( TcpUnixSocket::connect( path) );
    }
};

/// Class representing a TCP internet server socket
class TcpInetServer : public TcpInetSocket
{
public:
    bool bind( const char *host, const char *serv )
    {
        return( TcpInetSocket::bind( host, serv ) );
    }
    bool bind( const char *host, int port )
    {
        return( TcpInetSocket::bind( host, port ) );
    }
    bool bind( const char *serv )
    {
        return( TcpInetSocket::bind( serv ) );
    }
    bool bind( int port )
    {
        return( TcpInetSocket::bind( port ) );
    }

public:
    bool isListening() const { return( Socket::isListening() ); }
    bool listen();
    bool accept();
    bool accept( TcpInetSocket *&newSocket );
};

/// Class representing a TCP Unix server socket
class TcpUnixServer : public TcpUnixSocket
{
public:
    bool bind( const char *path )
    {
        return( TcpUnixSocket::bind( path ) );
    }

public:
    bool isListening() const { return( Socket::isListening() ); }
    bool listen();
    bool accept();
    bool accept( TcpUnixSocket *&newSocket );
};

/// Class representing the Unxi select system call that waits on sockets for notifications of readiness to receive or transmit data
class Select
{
public:
    typedef std::set<CommsBase *> CommsSet;
    typedef std::vector<CommsBase *> CommsList;

protected:
    CommsSet        mReaders;
    CommsSet        mWriters;
    CommsList       mReadable;
    CommsList       mWriteable;
    bool            mHasTimeout;
    struct timeval  mTimeout;
    int             mMaxFd;

public:
    Select();
    Select( struct timeval timeout );
    Select( int timeout );
    Select( double timeout );

    void setTimeout( int timeout );
    void setTimeout( double timeout );
    void setTimeout( struct timeval timeout );
    void clearTimeout();

    void calcMaxFd();

    bool addReader( CommsBase *comms );
    bool deleteReader( CommsBase *comms );
    void clearReaders();

    bool addWriter( CommsBase *comms );
    bool deleteWriter( CommsBase *comms );
    void clearWriters();

    int wait();

    const CommsList &getReadable() const;
    const CommsList &getWriteable() const;
};

#endif // LIBGEN_COMMS_H

#include "zmListener.h"

#include "zmConnection.h"
#include "../protocols/zmHttpController.h"
#include "../protocols/zmRtspController.h"
#include "../protocols/zmRtmpController.h"

/**
* @brief 
*
* @param interface
*/
Listener::Listener( const std::string &interface ) :
    Thread( "Listener" ),
    mInterface( interface )
{
}

/**
* @brief 
*/
Listener::~Listener()
{
}

/**
* @brief 
*
* @param controller
*/
void Listener::addController( Controller *controller )
{
    std::pair<ControllerMap::iterator,bool> result = mControllers.insert( ControllerMap::value_type( controller->port(), controller ) );
    if ( !result.second )
        Fatal( "Unable to add controller %s on port %d", controller->cproto(), controller->port() );
}

/**
* @brief 
*
* @param controller
*/
void Listener::removeController( Controller *controller )
{
    bool result = ( mControllers.erase( controller->port() ) == 1 );
    if ( !result )
        Fatal( "Unable to remove controller %s on port %d", controller->cproto(), controller->port() );
}
/**
* @brief 
*
* @return 
*/
int Listener::run()
{
    typedef std::map<TcpInetServer *,Controller *>   SocketControllers;
    
    Debug( 2, "Listener running" );
    Select listener( 5 );
    
    SocketControllers socketControllers;
    for ( ControllerMap::iterator iter = mControllers.begin(); iter != mControllers.end(); iter++ )
    {
        Controller *controller = iter->second;
        TcpInetServer *socket = new TcpInetServer();
        socketControllers.insert( SocketControllers::value_type( socket, controller ) );
        if ( mInterface == "any" )
            socket->bind( controller->port() );
        else
            socket->bind( mInterface.c_str(), controller->port() );
        socket->listen();
        listener.addReader( socket );
    }

    while( !mStop && (listener.wait() >= 0) )
    {
        Select::CommsList socketList = listener.getReadable();

        for ( Select::CommsList::iterator iter = socketList.begin(); iter != socketList.end(); iter++ )
        {
            TcpInetSocket *thisSocket = dynamic_cast<TcpInetSocket *>(*iter);
            if ( thisSocket )
            {
                SocketControllers::iterator iter = socketControllers.find( dynamic_cast<TcpInetServer *>(thisSocket) );
                if ( iter != socketControllers.end() )
                {
                    TcpInetServer *socket = iter->first;
                    Controller *controller = iter->second;
                    Debug( 3, "Got connection on controller %s socket", controller->cproto() );
                    TcpInetSocket *newSocket = 0;
                    if ( socket->accept( newSocket ) )
                    {
                        Debug( 3, "New %s session, remote host %s", controller->cproto(), newSocket->getRemoteAddr()->host().c_str() );
                        Connection *connection = controller->newConnection( newSocket );
                        mConnections.insert( Connections::value_type( newSocket, connection ) );
                        Debug( 3, "Got %zd connections", mConnections.size() );
                        listener.addReader( newSocket );
                    }
                }
                else
                {
                    Debug( 3, "Got data on existing connection" );
                    Connections::iterator connectionIter = mConnections.find( thisSocket );
                    if ( connectionIter == mConnections.end() )
                    {
                        Error( "Unable to find Connection for socket at %p", thisSocket );
                        thisSocket->close();
                        delete thisSocket;
                        continue;
                    }
                    Connection *connection = connectionIter->second;

                    ssize_t nBytes = thisSocket->bytesToRead();
                    if ( nBytes < 0 )
                    {
                        Error( "Unable to get number of bytes in queue" );
                    }
                    else if ( nBytes == 0 )
                    {
                        Debug( 3, "Zero length queue, socket closed" );
                        listener.deleteReader( thisSocket );
                        mConnections.erase( connectionIter );
                        Debug( 3, "Got %zd connections", mConnections.size() );
                        delete connection;
                        thisSocket->close();
                        delete thisSocket;
                    }
                    else
                    {
                        bool result = false;
                        Debug( 3, "Got %zd bytes to recv", nBytes );
                        if ( connection->isText() )
                        {
                            std::string request;
                            int status = thisSocket->recv( request, nBytes );
                            if ( status > 0 )
                            {
                                Debug( 3, "Got request '%s'", request.c_str() );
                                if ( connection->recvRequest( request ) )
                                    result = true;
                            }
                            else
                            {
                                Error( "Unable to recv request, status is %d", status );
                            }
                        }
                        else if ( connection->isBinary() )
                        {
                            ByteBuffer request( nBytes );
                            int status = thisSocket->recv( request.data(), request.size() );
                            if ( status > 0 )
                            {
                                Debug( 3, "Got request, %d bytes", status );
                                request.size( status );
                                if ( connection->recvRequest( request ) )
                                    result = true;
                            }
                            else
                            {
                                Error( "Unable to recv request, status is %d", status );
                            }
                        }
                        else
                        {
                            Panic( "No connection format defined" );
                        }
                        if ( !result )
                        {
                            listener.deleteReader( thisSocket );
                            mConnections.erase( connectionIter );
                            Debug( 3, "Got %zd connections", mConnections.size() );
                            delete connection;
                            thisSocket->close();
                            delete thisSocket;
                        }
                    }
                }
            }
            else
            {
                Fatal( "Unable to cast socket to acceptable type" );
            }
        }
    }
    return( 0 );
}

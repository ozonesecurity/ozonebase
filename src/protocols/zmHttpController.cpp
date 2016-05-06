#include "../zm.h"
#include "zmHttpController.h"

#include "zmHttp.h"
#include "zmHttpSession.h"
#include "zmHttpConnection.h"

Connection *HttpController::newConnection( TcpInetSocket *socket )
{
    return( new HttpConnection( this, socket ) );
}
     
HttpSession *HttpController::getSession( uint32_t session )
{
    HttpSessions::iterator iter = mHttpSessions.find( session );
    if ( iter == mHttpSessions.end() )
    {
        Error( "Can't find HTTP session %x", session );
        return( 0 );
    }
    return( iter->second );
}

// Create a new HTTP session
HttpSession *HttpController::newSession( uint32_t session )
{
    HttpSession *httpSession = new HttpSession( session );
    mHttpSessions.insert( HttpSessions::value_type( session, httpSession ) );

    return( httpSession );
}

void HttpController::deleteSession( uint32_t session )
{
    HttpSessions::iterator iter = mHttpSessions.find( session );
    if ( iter != mHttpSessions.end() )
    {
        delete iter->second;
        mHttpSessions.erase( iter );
    }
}

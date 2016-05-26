#include "../base/zm.h"
#include "zmRtmpController.h"

#include "zmRtmp.h"
#include "zmRtmpSession.h"
#include "zmRtmpConnection.h"

Connection *RtmpController::newConnection( TcpInetSocket *socket )
{
    return( new RtmpConnection( this, socket ) );
}

RtmpSession *RtmpController::getSession( uint32_t session )
{
    RtmpSessions::iterator iter = mRtmpSessions.find( session );
    if ( iter == mRtmpSessions.end() )
    {
        Debug( 2, "Can't find RTMP session %x", session );
        return( 0 );
    }
    return( iter->second );
}

// Create a new RTMP session
RtmpSession *RtmpController::newSession( uint32_t session, RtmpConnection *connection )
{
    Debug( 2, "Creating RTMP session %x", session );
    RtmpSession *rtmpSession = new RtmpSession( session, connection );
    mRtmpSessions.insert( RtmpSessions::value_type( session, rtmpSession ) );

    return( rtmpSession );
}

void RtmpController::deleteSession( uint32_t session )
{
    Debug( 2, "Deleting RTMP session %x", session );
    RtmpSessions::iterator iter = mRtmpSessions.find( session );
    if ( iter != mRtmpSessions.end() )
    {
        delete iter->second;
        mRtmpSessions.erase( iter );
    }
}

#include "../zm.h"
#include "zmRtspController.h"

#include "zmRtsp.h"
#include "zmRtspSession.h"
#include "zmRtspConnection.h"

#include <stdlib.h>

RtspController::RtspController( const std::string &name, uint32_t tcpPort, const PortRange &udpPortRange ) :
    Controller( cClass(), name, "RTSP", tcpPort ),
    mDataPortRange( udpPortRange )
{
    Debug( 2, "Assigned RTP port range is %d-%d", mDataPortRange.first, mDataPortRange.second );
}

RtspController::~RtspController()
{
}

Connection *RtspController::newConnection( TcpInetSocket *socket )
{
    return( new RtspConnection( this, socket ) );
}

RtspSession *RtspController::getSession( uint32_t session )
{
    RtspSessions::iterator iter = mRtspSessions.find( session );
    if ( iter == mRtspSessions.end() )
    {
        Error( "Can't find RTSP session %x", session );
        return( 0 );
    }
    return( iter->second );
}

// Create a new RTSP session
RtspSession *RtspController::newSession( RtspConnection *connection, Encoder *encoder )
{
    uint32_t session = 0;

    // Prevent duplicate sessions
    RtspSessions::iterator iter;
    do
    {
        session = rand();
        iter = mRtspSessions.find( session );
    }
    while( iter != mRtspSessions.end() );
    
    RtspSession *rtspSession = new RtspSession( session, connection, encoder );
    mRtspSessions.insert( RtspSessions::value_type( session, rtspSession ) );

    return( rtspSession );
}

// Assign RTP ports from pool making sure not to use ones
// already in use. Ports always assigned as pairs
int RtspController::requestPorts()
{
    for ( int i = mDataPortRange.first; i <= mDataPortRange.second; i += 2 )
    {
        PortSet::const_iterator iter = mAssignedPorts.find( i );
        if ( iter == mAssignedPorts.end() )
        {
            mAssignedPorts.insert( i );
            return( i );
        }
    }
    Panic( "Can't assign RTP port, no ports left in pool" );
    return( -1 );
}

// Return port(s) to pool
void RtspController::releasePorts( int port )
{
    if ( port > 0 )
        mAssignedPorts.erase( port );
}

// Get unique random RTP Synchronisation Source identifier
uint32_t RtspController::requestSsrc()
{
    // Prevent duplicate sessions
    uint32_t ssrc = 0;
    SsrcSet::iterator iter;
    do
    {
        ssrc = rand();
        iter = mSsrcs.find( ssrc );
    }
    while( iter != mSsrcs.end() );
    mSsrcs.insert( ssrc );
    return( ssrc );
}

void RtspController::releaseSsrc( uint32_t ssrc )
{
    mSsrcs.erase( ssrc );
}


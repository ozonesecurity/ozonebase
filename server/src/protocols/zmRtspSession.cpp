#include "../base/zm.h"
#include "zmRtspSession.h"

#include "zmRtsp.h"
#include "zmRtspController.h"
#include "zmRtspStream.h"
#include "../base/zmFeedProvider.h"

#include "../libgen/libgenUtils.h"

/**
* @brief 
*
* @param session
* @param connection
* @param encoder
*/
RtspSession::RtspSession( int session, RtspConnection *connection, Encoder *encoder ) :
    mState( INIT ),
    mSession( session ),
    mConnection( connection ),
    mEncoder( encoder )
{
    Debug( 2, "New RTSP session %d", session );
}

/**
* @brief 
*/
RtspSession::~RtspSession()
{
}

// Handle RTSP commands once session established.
// Otherwise RTSP commands are handled in RTSP connection class
/**
* @brief 
*
* @param requestType
* @param requestUrl
* @param requestHeaders
* @param responseHeaders
*
* @return 
*/
bool RtspSession::recvRequest( const std::string &requestType, const std::string &requestUrl, const Connection::Headers &requestHeaders, Connection::Headers &responseHeaders )
{
    Debug( 2, "Session %d - Processing RTSP request: %s", mSession, requestType.c_str() );

    if ( requestType == "SETUP" )
    {
        std::string requestHost, requestStreamName, requestStreamSource;
        int trackId = -1;
        FeedProvider *provider = mConnection->validateRequestUrl( requestUrl, &requestHost, &requestStreamName, &requestStreamSource, &trackId );
        if ( !provider )
        {
            mConnection->sendResponse( responseHeaders, "", 404, "Not Found" );
            return( false );
        }
        if ( !provider->ready() )
        {
            mConnection->sendResponse( responseHeaders, "", 503, "Stream not ready" );
            return( false );
        }
 
        if ( trackId < 0 || trackId > 1 )
        {
            Error( "Got invalid trackID %d from request URL '%s'", trackId, requestUrl.c_str() );
            return( false );
        }

        if ( mRtspStreams.find( trackId ) != mRtspStreams.end() )
        {
            Error( "Got RTSP SETUP for existing trackID %d", trackId );
            return( false );
        }

        Connection::Headers::const_iterator iter = requestHeaders.find("Transport");
        if ( iter == requestHeaders.end() )
        {
            Error( "No Transport header found in SETUP" );
            return( false );
        }
        std::string transportSpec = iter->second;

        RtspStream *stream = newStream( trackId, mEncoder, transportSpec );

        transportSpec.clear();
        transportSpec += "RTP/AVP";
        if ( stream->lowerTransport() == RtspStream::LOWTRANS_TCP )
        {
            transportSpec += "/TCP";
        }
        else
        {
            transportSpec += "/UDP";
            if ( stream->delivery() == RtspStream::DEL_UNICAST )
                transportSpec += ";unicast";
            else
                transportSpec += ";multicast";
        }
        if ( stream->interleaved() )
        {
            transportSpec += stringtf( ";interleaved=%d-%d", stream->channel(0), stream->channel(1) );
        }
        else
        {
            transportSpec += stringtf( ";client_port=%d-%d", stream->destPort(0), stream->destPort(1) );
            transportSpec += stringtf( ";server_port=%d-%d", stream->sourcePort(0), stream->sourcePort(1) );
        }
        transportSpec += stringtf( ";ssrc=%08X", stream->ssrc() );
        transportSpec += ";mode=\"PLAY\"";
        responseHeaders.insert( Connection::Headers::value_type( "Session", stringtf( "%08X", mSession ) ) );
        responseHeaders.insert( Connection::Headers::value_type( "Transport", transportSpec ) );

        state( READY );
        return( true );
    }
    else if ( requestType == "PLAY" )
    {
        std::string rtpInfo;
        for ( RtspStreams::iterator iter = mRtspStreams.begin(); iter != mRtspStreams.end(); iter++ )
        {
            const RtspStream *stream = iter->second;
            if ( rtpInfo.size() )
                rtpInfo += ",";
            rtpInfo += stringtf( "url=%s/trackID=%d;seq=0;rtptime=0", requestUrl.c_str(), stream->trackId() );
        }
        responseHeaders.insert( Connection::Headers::value_type( "Session", stringtf( "%08X", mSession ) ) );
        responseHeaders.insert( Connection::Headers::value_type( "Range", "npt=0-" ) );
        responseHeaders.insert( Connection::Headers::value_type( "RTP-Info", rtpInfo ) );
        for ( RtspStreams::iterator iter = mRtspStreams.begin(); iter != mRtspStreams.end(); iter++ )
            iter->second->start();
        state( PLAYING );
        return( true );
    }
    else if ( requestType == "GET_PARAMETER" )
    {
        FeedProvider *provider = mConnection->validateRequestUrl( requestUrl );
        if ( provider )
        {
            responseHeaders.insert( Connection::Headers::value_type( "Session", stringtf( "%08X", mSession ) ) );
            return( true );
        }
        mConnection->sendResponse( responseHeaders, "", 451, "Parameter Not Understood" );
        return( false );
    }
    else if ( requestType == "TEARDOWN" )
    {
        Debug( 1, "Stopping streams" );
        for ( RtspStreams::iterator iter = mRtspStreams.begin(); iter != mRtspStreams.end(); iter++ )
            iter->second->stop();
        Debug( 1, "Waiting for streams" );
        for ( RtspStreams::iterator iter = mRtspStreams.begin(); iter != mRtspStreams.end(); iter++ )
            iter->second->join();
        for ( RtspStreams::iterator iter = mRtspStreams.begin(); iter != mRtspStreams.end(); iter++ )
            delete iter->second;
        mRtspStreams.clear();
        state( INIT );
        return( true );
    }
    Error( "Unrecognised RTSP command '%s'", requestType.c_str() );
    return( false );
}

// Create a new RTSP stream object from config received over RTSP
/**
* @brief 
*
* @param trackId
* @param encoder
* @param transportSpec
*
* @return 
*/
RtspStream *RtspSession::newStream( int trackId, Encoder *encoder, const std::string &transportSpec )
{
    StringTokenList transportParts( transportSpec, ";" );
    if ( transportParts.size() < 1 )
    {
        Error( "Unable to parse transport data from transport '%s'", transportSpec.c_str() );
        return( NULL );
    }

    const std::string &protocolSpec = transportParts[0];
    StringTokenList protocolParts( protocolSpec, "/" );
    if ( protocolParts.size() < 2 )
    {
        Error( "Unable to parse transport data from protocol '%s'", protocolSpec.c_str() );
        return( NULL );
    }

    const std::string transport = protocolParts[0];
    const std::string profile = protocolParts[1];
    const std::string lowerTransport = protocolParts.size()>2?protocolParts[2]:"UDP";

    Debug( 3, "Got transport '%s'", transport.c_str() );
    Debug( 3, "Got profile '%s'", profile.c_str() );
    Debug( 3, "Got lowerTransport '%s'", lowerTransport.c_str() );

    StringTokenList::TokenList tokens = transportParts.tokens();
    tokens.pop_front();

    RtspStream *rtspStream = new RtspStream( this, trackId, mConnection, encoder, transport, profile, lowerTransport, tokens );
    mRtspStreams.insert( RtspStreams::value_type( trackId, rtspStream ) );

    return( rtspStream );
}

/**
* @brief 
*
* @param newState
*
* @return 
*/
RtspSession::State RtspSession::state( State newState )
{
    Debug( 4, "RTSP Session %d changing to state %d", mSession, newState );
    return( mState = newState );
}

/**
* @brief 
*
* @return 
*/
int RtspSession::requestPorts()
{
    return( mConnection->controller()->requestPorts() );
}

/**
* @brief 
*
* @param port
*/
void RtspSession::releasePorts( int port )
{
    mConnection->controller()->releasePorts( port );
}

/**
* @brief 
*
* @return 
*/
uint32_t RtspSession::requestSsrc()
{
    return( mConnection->controller()->requestSsrc() );
}

/**
* @brief 
*
* @param ssrc
*/
void RtspSession::releaseSsrc( uint32_t ssrc )
{
    mConnection->controller()->releaseSsrc( ssrc );
}

#include "../base/zm.h"
#include "zmHttpSession.h"

#include "zmHttp.h"
#include "zmHttpController.h"
#include "zmHttpConnection.h"
#include "zmHttpStream.h"
#include "../base/zmFeedProvider.h"

#include "../libgen/libgenUtils.h"
#include <stdlib.h>

HttpSession::HttpSession( int session ) :
    mSession( session ),
    mHttpStream( 0 )
{
    Debug( 2, "New HTTP session %X", session );
}

HttpSession::~HttpSession()
{
    Debug( 1, "Closing HTTP session" );
    if ( mHttpStream )
    {
        Debug( 1, "Closing HTTP stream" );
        mHttpStream->stop();
        mHttpStream->join();
        delete mHttpStream;
        mHttpStream = 0;
    }
}

// Handle HTTP commands once session established.
// Otherwise HTTP commands are handled in HTTP connection class
bool HttpSession::recvRequest( HttpConnection *connection, const std::string &requestType, const std::string &requestUrl, const Connection::Headers &requestHeaders, Connection::Headers &responseHeaders )
{
    Debug( 2, "Session %X - Processing HTTP request: %s", mSession, requestType.c_str() );

    responseHeaders.insert( Connection::Headers::value_type( "Session", stringtf( "%X", mSession ) ) );

    if ( requestType == "GET" )
    {
        std::string streamName;
        std::string streamSource;
        int width = -1;
        int height = -1;
        int rate = 10;
        int quality = 80;

        size_t queryPos = requestUrl.find_first_of( '?' );
        if ( queryPos == std::string::npos )
        {
            StringTokenList paths( requestUrl, "/", StringTokenList::STLF_MULTI );
            if ( paths.size() == 2 )
            {
                streamName = paths[0];
                streamSource = paths[1];
            }
        }
        else
        {
            StringTokenList paths( requestUrl.substr( 0, queryPos ), "/", StringTokenList::STLF_MULTI );
            if ( paths.size() == 2 )
            {
                streamName = paths[0];
                streamSource = paths[1];
            }
            StringTokenList parameters( requestUrl.substr( queryPos+1 ), "&" );
            for ( int i = 0; i < parameters.size(); i++ )
            {
                StringTokenList parameter( parameters[i], "=" );
                const std::string &name = parameter[0];
                const std::string &value = parameter[1];
                if ( name == "width" )
                    width = atoi( value.c_str() );
                else if ( name == "height" )
                    height = atoi( value.c_str() );
                else if ( name == "rate" )
                    rate = atoi( value.c_str() );
                else if ( name == "quality" )
                    quality = atoi( value.c_str() );
                else
                    Error( "Unrecognised query parameter %s", parameters[i].c_str() );
            }
        }
        FeedProvider *streamProvider = connection->controller()->findStream( streamName, streamSource );
        if ( !streamProvider )
        {
            connection->sendResponse( responseHeaders, "", 404, "Invalid Source" );
            return( false );
        }
        if ( !streamProvider->ready() )
        {
            connection->sendResponse( responseHeaders, "", 503, "Stream not ready" );
            return( false );
        }
        if ( width == -1 || height == -1 )
        {
            const VideoProvider *videoProvider = dynamic_cast<const VideoProvider *>(streamProvider);
            width = videoProvider->width();
            height = videoProvider->height();
        }
        responseHeaders.insert( Connection::Headers::value_type( "Cache-Control", "no-cache" ) );
        responseHeaders.insert( Connection::Headers::value_type( "Pragma", "no-cache" ) );
        responseHeaders.insert( Connection::Headers::value_type( "Expires-Control", "Thu, 01 Dec 1994 16:00:00 GMT" ) );
        responseHeaders.insert( Connection::Headers::value_type( "Access-Control-Allow-Origin", "*" ) );

        if ( streamProvider->hasVideo() )
        {
            responseHeaders.insert( Connection::Headers::value_type( "Connection", "close" ) );
            responseHeaders.insert( Connection::Headers::value_type( "Content-Type", "multipart/x-mixed-replace; boundary=--imgboundary" ) );
            mHttpStream = new HttpImageStream( this, connection, streamProvider, width, height, FrameRate( rate ), quality );
        }
        else
            mHttpStream = new HttpDataStream( this, connection, streamProvider );

        mHttpStream->start();

        return( true );
    }
    Error( "Unrecognised HTTP command '%s'", requestType.c_str() );
    connection->sendResponse( responseHeaders, "", 405, "Method Not Allowed" );
    return( false );
}

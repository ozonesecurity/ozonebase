#include "../base/oz.h"
#include "ozHttpConnection.h"

#include "ozHttp.h"
#include "ozHttpSession.h"
#include "ozHttpStream.h"
#include "../libgen/libgenUtils.h"

/**
* @brief 
*
* @param controller
* @param socket
*/
HttpConnection::HttpConnection( Controller *controller, TcpInetSocket *socket ) :
    TextConnection( controller, socket ),
    mHttpController( dynamic_cast<HttpController *>(controller) )
{
}

/**
* @brief 
*/
HttpConnection::~HttpConnection()
{
    mHttpController->deleteSession( (uintptr_t)this );
}

// Transmit an HTTP response message over an established connection
/**
* @brief 
*
* @param headers
* @param payload
* @param statusCode
* @param statusText
*
* @return 
*/
bool HttpConnection::sendResponse( Headers &headers, std::string payload, int statusCode, std::string statusText )
{
    std::string response;

    response = stringtf( "HTTP/1.0 %d %s\r\n", statusCode, statusText.c_str() );

    headers.insert( Headers::value_type( "Server", "Test HTTP Server V1.0" ) );
    for ( Headers::iterator iter = headers.begin(); iter != headers.end(); iter++ )
    {
        response += iter->first+": "+iter->second+"\r\n";
    }
    response += "\r\n";

    if ( payload.size() )
    {
        response += payload;
    }

    Debug( 2, "Sending HTTP response: %s", response.c_str() );

    if ( mSocket->send( response.c_str(), response.size() ) != (int)response.length() )
    {
        Error( "Unable to send response '%s': %s", response.c_str(), strerror(errno) );
        return( false );
    }

    return( true );
}

// Receive, assemble and handle HTTP commands that are not related to established streams
/**
* @brief 
*
* @param request
*
* @return 
*/
bool HttpConnection::recvRequest( std::string &request )
{
    Debug( 2, "Received HTTP request: %s (%zd bytes)", request.c_str(), request.size() );

    if ( mRequest.size() > 0 )
    {
        request = mRequest+request;
        mRequest.clear();
        Debug( 6, "Merging with saved request, total: %s", request.c_str() );
    }

    StringTokenList lines( request, "\r\n" );
    if ( lines.size() <= 0 )
    {
        Error( "Unable to split request '%s' into tokens", request.c_str() );
        return( false );
    }
    if ( lines[lines.size()-1].size() != 0 )
    {
        Error( "Request '%s' is incomplete, storing", request.c_str() );
        mRequest = request;
        return( true );
    }

    StringTokenList parts( lines[0], " " );
    if ( parts.size() != 3 )
    {
        Error( "Unable to split request part '%s' into tokens", lines[0].c_str() );
        return( false );
    }

    std::string requestType = parts[0];
    Debug( 4, "Got request '%s'", requestType.c_str() );
    std::string requestUrl = parts[1];
    Debug( 4, "Got requestUrl '%s'", requestUrl.c_str() );
    std::string requestVer = parts[2];
    Debug( 4, "Got requestVer '%s'", requestVer.c_str() );
    if ( requestVer != "HTTP/1.0" && requestVer != "HTTP/1.1" )
    {
        Error( "Unexpected HTTP version '%s'", requestVer.c_str() );
        return( false );
    }

    // Extract headers from request
    Headers requestHeaders;
    for ( int i = 1; i < (lines.size()-1); i++ )
    {
        StringTokenList parts( lines[i], ": " );
        if ( parts.size() != 2 )
        {
            Error( "Unable to split request header '%s' into tokens", lines[i].c_str() );
            return( false );
        }
        Debug( 4, "Got header '%s', value '%s'", parts[0].c_str(), parts[1].c_str() );
        requestHeaders.insert( Headers::value_type( parts[0], parts[1] ) );
    }

    Headers responseHeaders;
    if ( requestType == "GET" )
    {
        StringTokenList urlParts( requestUrl, "/?&",  StringTokenList::STLF_ALTCHARS|StringTokenList::STLF_MULTI );
        const std::string &streamName = urlParts[0];
        //const std::string &streamSource = urlParts[1];
        if ( !mController->verifyStreamName( streamName ) )
        {
            Error( "Invalid request stream name '%s'", streamName.c_str() );
            return( sendResponse( responseHeaders, "", 404, "Not Found" ) );
        }
        // These commands are handled by HTTP session so pass them on and send any required responses
        HttpSession *httpSession = mHttpController->newSession( (uintptr_t)this );
        if ( httpSession->recvRequest( this, requestType, requestUrl, requestHeaders, responseHeaders ) )
            return( sendResponse( responseHeaders ) );
        return( false );
    }
    Error( "Unrecognised HTTP method '%s'", requestType.c_str() );
    return( sendResponse( responseHeaders, "", 405, "Method not allowed" ) );
}


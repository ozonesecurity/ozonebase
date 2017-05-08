#include "../base/oz.h"
#include "ozRtspConnection.h"

#include "ozRtsp.h"
#include "ozRtspSession.h"
#include "ozRtpSession.h"
#include "ozRtpData.h"
#include "../base/ozEncoder.h"
#include "../encoders/ozH264Encoder.h"
#include "../encoders/ozH264Relay.h"
#include "../encoders/ozMpegEncoder.h"

#include "../libgen/libgenUtils.h"
#include "../libgen/libgenTime.h"

#include <sys/uio.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

/**
* @brief 
*
* @param controller
* @param socket
*/
RtspConnection::RtspConnection( Controller *controller, TcpInetSocket *socket ) :
    BinaryConnection( controller, socket ),
    mRtspController( dynamic_cast<RtspController *>( controller ) ),
    mEncoder( NULL )
{
}

/**
* @brief 
*
* @param url
* @param host
* @param streamName
* @param streamSource
* @param track
*
* @return 
*/
FeedProvider *RtspConnection::validateRequestUrl( const std::string &url, std::string *host, std::string *streamName, std::string *streamSource, int *track )
{
    char tempHost[64]="", tempStreamName[64]="", tempStreamSource[64]="";
    int tempTrackId = -1;

    if ( sscanf( url.c_str(), "rtsp://%[a-zA-Z0-9_-.:]/%[^/]/%[^/]/trackID=%d", tempHost, tempStreamName, tempStreamSource, &tempTrackId ) != 4 )
    {
        if ( sscanf( url.c_str(), "rtsp://%[a-zA-Z0-9_-.:]/%[^/]/%[^/]", tempHost, tempStreamName, tempStreamSource ) != 3 )
        {
            Error( "Unable to parse request URL '%s'", url.c_str() );
            return( NULL );
        }
    }

    if ( host )
        *host = tempHost;
    if ( streamName )
        *streamName = tempStreamName;
    if ( streamSource )
        *streamSource = tempStreamSource;
    if ( track )
        *track = tempTrackId;

    if ( !mController->verifyStreamName( tempStreamName ) )
    {
        Error( "Invalid request streamName '%s'", tempStreamName );
        return( NULL );
    }
    FeedProvider *provider = mController->findStream( tempStreamName, tempStreamSource );
    if ( !provider )
    {
        Error( "Invalid request stream '%s/%s'", tempStreamName, tempStreamSource );
        return( NULL );
    }
    return( provider );
}

// Handle interleaved RTP commands that are not related to established streams
/**
* @brief 
*
* @param buffer
*
* @return 
*/
bool RtspConnection::handleRequest( const ByteBuffer &buffer )
{
    int channel= buffer[1];
    uint16_t length = buffer.size()-4;
    Debug( 2, "Got %d byte packet on channel %d", length, channel );
    Hexdump( 2, buffer+4, length );
    return( true );
}

// Handle RTSP commands that are not related to established streams
/**
* @brief 
*
* @param request
*
* @return 
*/
bool RtspConnection::handleRequest( const std::string &request )
{
    Debug( 2, "Handling RTSP request: %s (%zd bytes)", request.c_str(), request.size() );

    StringTokenList lines( request, "\r\n" );
    if ( lines.size() <= 0 )
    {
        Error( "Unable to split request '%s' into tokens", request.c_str() );
        return( false );
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
    if ( requestVer != "RTSP/1.0" )
    {
        Error( "Unexpected RTSP version '%s'", requestVer.c_str() );
        return( false );
    }

    // Extract headers from request
    Headers requestHeaders;
    for ( int i = 1; i < lines.size(); i++ )
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

    if ( requestHeaders.find("CSeq") == requestHeaders.end() )
    {
        Error( "No CSeq header found" );
        return( false );
    }
    Debug( 4, "Got sequence number %s", requestHeaders["CSeq"].c_str() );

    uint32_t session = 0;
    if ( requestHeaders.find("Session") != requestHeaders.end() )
    {
        Debug( 4, "Got session header, '%s', passing to session", requestHeaders["Session"].c_str() );
        session = strtol( requestHeaders["Session"].c_str(), NULL, 16 );
    }

    Headers responseHeaders;
    responseHeaders.insert( Headers::value_type( "CSeq", requestHeaders["CSeq"] ) );
    if ( requestType == "OPTIONS" )
    {
        responseHeaders.insert( Headers::value_type( "Public", "DESCRIBE, SETUP, PLAY, GET_PARAMETER, TEARDOWN" ) );
        return( sendResponse( responseHeaders ) );
    }
    else if ( requestType == "DESCRIBE" )
    {
        FeedProvider *provider = validateRequestUrl( requestUrl );

        if ( !provider )
        {
            sendResponse( responseHeaders, "", 404, "Not Found" );
            return( false );
        }

        const VideoProvider *videoProvider = dynamic_cast<const VideoProvider *>(provider);

        int codec = AV_CODEC_ID_MPEG4;
        int width = videoProvider->width();
        int height = videoProvider->height();
        FrameRate frameRate = 15;
        //FrameRate frameRate = videoProvider->frameRate();
        int bitRate = 90000;
        int quality = 70;

        std::string sdpFormatString =
            "v=0\r\n"
            "o=- %jd %jd IN IP4 %s\r\n"
            "s=ZoneMinder Stream\r\n"
            "i=Media Streamers\r\n"
            "c=IN IP4 0.0.0.0\r\n"
            "t=0 0\r\n"
            "a=control:*\r\n"
            "a=range:npt=0.000000-\r\n";
        uint64_t now64 = time64();
        char hostname[HOST_NAME_MAX] = "";
        if ( gethostname( hostname, sizeof(hostname) ) < 0 )
            Fatal( "Can't gethostname: %s", strerror(errno) );

        std::string sdpString = stringtf( sdpFormatString, now64, now64, hostname );

        if ( codec == AV_CODEC_ID_H264 )
        {
            if ( provider->cl4ss() == "RawH264Input" )
            {
                std::string encoderKey = H264Relay::getPoolKey( provider->identity(), width, height, frameRate, bitRate, quality );
                if ( !(mEncoder = Encoder::getPooledEncoder( encoderKey )) )
                {
                    H264Relay *h264Relay = NULL;
                    mEncoder = h264Relay = new H264Relay( provider->identity(), width, height, frameRate, bitRate, quality );
                    mEncoder->registerProvider( *provider );
                    Encoder::poolEncoder( mEncoder );
                    h264Relay->start();
                }
                sdpString += mEncoder->sdpString( 1 ); // XXX - Should be variable
                responseHeaders.insert( Headers::value_type( "Content-length", stringtf( "%zd", sdpString.length() ) ) );
            }
            else
            {
                std::string encoderKey = H264Encoder::getPoolKey( provider->identity(), width, height, frameRate, bitRate, quality );
                if ( !(mEncoder = Encoder::getPooledEncoder( encoderKey )) )
                {
                    H264Encoder *h264Encoder = NULL;
                    mEncoder = h264Encoder = new H264Encoder( provider->identity(), width, height, frameRate, bitRate, quality );
                    mEncoder->registerProvider( *provider );
                    Encoder::poolEncoder( mEncoder );
                    h264Encoder->start();
                }
                sdpString += mEncoder->sdpString( 1 ); // XXX - Should be variable
                responseHeaders.insert( Headers::value_type( "Content-length", stringtf( "%zd", sdpString.length() ) ) );
            }
        }
        else if ( codec == AV_CODEC_ID_MPEG4 )
        {
            std::string encoderKey = MpegEncoder::getPoolKey( provider->identity(), width, height, frameRate, bitRate, quality );
            if ( !(mEncoder = Encoder::getPooledEncoder( encoderKey )) )
            {
                MpegEncoder *mpegEncoder = NULL;
                mEncoder = mpegEncoder = new MpegEncoder( provider->identity(), width, height, frameRate, bitRate, quality );
                mEncoder->registerProvider( *provider );
                Encoder::poolEncoder( mEncoder );
                mpegEncoder->start();
            }
            sdpString += mEncoder->sdpString( 1 ); // XXX - Should be variable
            responseHeaders.insert( Headers::value_type( "Content-length", stringtf( "%zd", sdpString.length() ) ) );
        }
        return( sendResponse( responseHeaders, sdpString ) );
    }
    else if ( requestType == "SETUP" )
    {
        // These commands are handled by RTSP session so pass them on and send any required responses
        RtspSession *rtspSession = 0;
        if ( session )
        {
            rtspSession = mRtspController->getSession( session );
        }
        else
        {
            rtspSession = mRtspController->newSession( this, mEncoder );
        }
        if ( rtspSession->recvRequest( requestType, requestUrl, requestHeaders, responseHeaders ) )
            return( sendResponse( responseHeaders ) );
        return( false );
    }
    else if ( requestType == "PLAY" || requestType == "GET_PARAMETER" || requestType == "TEARDOWN" )
    {
        // These commands are handled by RTSP session so pass them on and send any required responses
        RtspSession *rtspSession = 0;
        if ( session )
        {
            rtspSession = mRtspController->getSession( session );
            if ( rtspSession && rtspSession->recvRequest( requestType, requestUrl, requestHeaders, responseHeaders ) )
                return( sendResponse( responseHeaders ) );
        }
        return( sendResponse( responseHeaders, "", 454, "Session Not Found" ) );
    }
    Error( "Unrecognised RTSP command '%s'", requestType.c_str() );
    return( sendResponse( responseHeaders, "", 405, "Method not implemented" ) );
}

// Receive, assemble and handle RTSP commands that are not related to established streams
/**
* @brief 
*
* @param buffer
*
* @return 
*/
bool RtspConnection::recvRequest( ByteBuffer &buffer )
{
    Debug( 2, "Received RTSP message, %zd bytes", buffer.size() );

    while( !buffer.empty() )
    {
        if ( buffer[0] == '$' || (!mRequest.empty() && mRequest[0] == '$') )
        {
            // Interleaved request
            if ( mRequest )
            {
                buffer = mRequest+buffer;
                mRequest.clear();
                Debug( 6, "Merging with saved request, total bytes: %zd", buffer.size() );
            }
            Debug( 2, "Got RTP interleaved request" );
            if ( buffer.size() <= 4 )
            {
                Debug( 6, "Request is incomplete, storing" );
                mRequest = buffer;
                return( true );
            }
            //int channel= buffer[1];
            uint16_t rawLength;
            memcpy( &rawLength, &buffer[2], sizeof(rawLength) );
            uint16_t length = be16toh( rawLength );
            if ( (length+2) > buffer.size() )
            {
                Debug( 6, "Request is incomplete, storing" );
    Hexdump( 2, buffer.data(), buffer.size() );
                mRequest = buffer;
                return( true );
            }
            ByteBuffer request = buffer.range( 0, 4+length );
            buffer.consume( 4+length );
            if ( !handleRequest( request ) )
                return( false );
        }
        else
        {
            Debug( 2, "Got RTSP request" );

            std::string request( reinterpret_cast<const char *>(buffer.data()), buffer.size() );

            if ( mRequest.size() > 0 )
            {
                //request = mRequest+request;
                request = std::string( reinterpret_cast<const char *>(mRequest.data()), mRequest.size() )+request;
                mRequest.clear();
                Debug( 6, "Merging with saved request, total: %s", request.c_str() );
            }

            const char *endOfMessage = strstr( request.c_str(), "\r\n\r\n" );

            if ( !endOfMessage )
            {
                Debug( 6, "Request '%s' is incomplete, storing", request.c_str() );
                mRequest = ByteBuffer( reinterpret_cast<const unsigned char *>(request.data()), request.size() );
                return( true );
            }

            size_t messageLen = endOfMessage-request.c_str();
            request.erase( messageLen );
            buffer.consume( messageLen+4 );
            if ( !handleRequest( request ) )
                return( false );
        }
    }
    return( true );
}

// Transmit an RTSP response message over an established connection
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
bool RtspConnection::sendResponse( Headers &headers, std::string payload, int statusCode, std::string statusText )
{
    std::string response;

    response = stringtf( "RTSP/1.0 %d %s\r\n", statusCode, statusText.c_str() );

    //headers.insert( Headers::value_type( "Server", "ZoneMinder Streamer V2.0" ) );
    headers.insert( Headers::value_type( "Date", datetimeInet() ) );
    for ( Headers::iterator iter = headers.begin(); iter != headers.end(); iter++ )
    {
        response += iter->first+": "+iter->second+"\r\n";
    }
    response += "\r\n";

    if ( payload.size() )
    {
        response += payload;
    }

    Debug( 2, "Sending RTSP response: %s", response.c_str() );

    if ( mSocket->send( response.c_str(), response.size() ) != (int)response.length() )
    {
        Error( "Unable to send response '%s': %s", response.c_str(), strerror(errno) );
        return( false );
    }

#if 0
    // Not currently used. If RTSP over HTTP ever required then will need to do something like this
    if ( mMethod == RTP_OZ_RTSP_HTTP )
    {
        response = base64Encode( response );
        Debug( 2, "Sending encoded RTSP response: %s", response.c_str() );
        if ( mRtspSocket2.send( response.c_str(), response.size() ) != (int)response.length() )
        {
            Error( "Unable to send response '%s': %s", response.c_str(), strerror(errno) );
            return( false );
        }
    }
    else
    {
        if ( mRtspSocket.send( response.c_str(), response.size() ) != (int)response.length() )
        {
            Error( "Unable to send response '%s': %s", response.c_str(), strerror(errno) );
            return( false );
        }
    }
#endif
    return( true );
}

/**
* @brief 
*
* @param channel
* @param packet
*
* @return 
*/
bool RtspConnection::sendInterleavedPacket( uint8_t channel, const ByteBuffer &packet )
{
    unsigned char header[4] = "$";
    header[1] = channel;
    uint16_t length = htobe16(packet.size());
    memcpy( &header[2], &length, sizeof(length) );

    struct iovec iovecs[2];
    iovecs[0].iov_base = header;
    iovecs[0].iov_len = sizeof(header);
    iovecs[1].iov_base = packet.data();
    iovecs[1].iov_len = packet.size();

    if ( mSocket->writeV( iovecs, sizeof(iovecs)/sizeof(*iovecs) ) != (sizeof(header)+packet.size()) )
    {
        Error( "Unable to send interleaved packet of %zd bytes on channel %d: %s", sizeof(header)+packet.size(), channel, strerror(errno) );
        return( false );
    }
    return( true );
}


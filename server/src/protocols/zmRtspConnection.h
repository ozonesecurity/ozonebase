#ifndef ZM_RTSP_CONNECTION_H
#define ZM_RTSP_CONNECTION_H

//
// Class representing the RTSP connection
//

#include "../base/zmConnection.h"

class FeedProvider;
class RtspController;
class Encoder;

// Class representing a single TCP connection to the RTSP server. This does not correlate with a specific
// session but is used primarily to maintain state and content between requests, especially partial ones,
// and manage access to the socket.
class RtspConnection : public BinaryConnection
{
private:
    RtspController  *mRtspController;

    Encoder         *mEncoder;

public:
    RtspConnection( Controller *controller, TcpInetSocket *socket );
    RtspController *controller() const { return( mRtspController ); }

public:
    FeedProvider *validateRequestUrl( const std::string &url, std::string *host=NULL, std::string *app=NULL, std::string *stream=NULL, int *track=NULL );
    bool handleRequest( const ByteBuffer &buffer );
    bool handleRequest( const std::string &request );
    bool recvRequest( ByteBuffer &request );
    bool sendResponse( Headers &headers, std::string payload="", int statusCode=200, std::string statusText="OK" );
    bool sendInterleavedPacket( uint8_t channel, const ByteBuffer &packet );
};

#endif // ZM_RTSP_CONNECTION_H

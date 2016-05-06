#ifndef ZM_HTTP_CONNECTION_H
#define ZM_HTTP_CONNECTION_H

#include "../zmConnection.h"

// Class representing a single TCP connection to the HTTP server. This does not correlate with a specific
// session but is used primarily to maintain state and content between requests, especially partial ones,
// and manage access to the socket.
// TODO - Needs tidy up code in destructor to clean up sessions and streams

class HttpController;

class HttpConnection : public TextConnection
{
private:
    HttpController  *mHttpController;

public:
    HttpConnection( Controller *controller, TcpInetSocket *socket );
    ~HttpConnection();

public:
    bool recvRequest( std::string &request );
    bool sendResponse( Headers &headers, std::string payload="", int statusCode=200, std::string statusText="OK" );
};

#endif // ZM_HTTP_CONNECTION_H

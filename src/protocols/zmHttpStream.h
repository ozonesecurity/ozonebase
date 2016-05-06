#ifndef ZM_HTTP_STREAM_H
#define ZM_HTTP_STREAM_H

#include "../zmStream.h"
#include "../libgen/libgenThread.h"
#include "../libgen/libgenComms.h"
#include "../zmFfmpeg.h"

class HttpSession;

// Class representing an HTTP stream.
class HttpStream : public Stream, public Thread
{
CLASSID(HttpStream);

private:
    HttpSession *mHttpSession;      // Pointer to the current HTTP session

protected:
    bool sendFrame( Select::CommsList &writeable, FramePtr frame );

public:
    HttpStream( HttpSession *session, Connection *connection, FeedProvider *provider, uint16_t width, uint16_t height, FrameRate frameRate, uint8_t quality );
    ~HttpStream();

protected:
    int run();
};

#endif // ZM_HTTP_STREAM_H

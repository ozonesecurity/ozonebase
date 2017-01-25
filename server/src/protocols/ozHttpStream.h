/** @addtogroup Protocols */
/*@{*/


#ifndef OZ_HTTP_STREAM_H
#define OZ_HTTP_STREAM_H

#include "../base/ozStream.h"
#include "../libgen/libgenThread.h"
#include "../libgen/libgenComms.h"
#include "../base/ozFfmpeg.h"

class HttpSession;

// Class representing an HTTP stream.
class HttpStream : public Stream, public Thread
{
protected:
    HttpSession *mHttpSession;      // Pointer to the current HTTP session

protected:
    virtual bool sendFrame( Select::CommsList &writeable, const FramePtr &frame ) = 0;

protected:
    HttpStream( const std::string &tag, HttpSession *session, Connection *connection, FeedProvider *provider );

public:
    ~HttpStream();
    int run();
};

// Class representing an HTTP image stream.
class HttpImageStream : public HttpStream
{
CLASSID(HttpImageStream);

protected:
    bool sendFrame( Select::CommsList &writeable, const FramePtr &frame );

public:
    HttpImageStream( HttpSession *session, Connection *connection, FeedProvider *provider, uint16_t width, uint16_t height, FrameRate frameRate, uint8_t quality );
    ~HttpImageStream();
};

// Class representing an HTTP stream.
class HttpDataStream : public HttpStream
{
CLASSID(HttpDataStream);

protected:
    bool sendFrame( Select::CommsList &writeable, const FramePtr &frame );

public:
    HttpDataStream( HttpSession *session, Connection *connection, FeedProvider *provider );
    ~HttpDataStream();
};

#endif // OZ_HTTP_STREAM_H


/*@}*/

#include "zm.h"
#include "zmStream.h"

#include "zmConnection.h"
#include "zmEncoder.h"

Stream::Stream( const std::string &tag, const std::string &id, Connection *connection, FeedProvider *provider ) :
    VideoConsumer( tag, id ),
    mConnection( connection ),
    mProvider( provider ),
    mEncoder( NULL )
{
}

Stream::Stream( const std::string &tag, const std::string &id, Connection *connection, Encoder *encoder ) :
    VideoConsumer( tag, id ),
    mConnection( connection ),
    mProvider( encoder->provider() ),
    mEncoder( encoder )
{
}

Stream::~Stream()
{
}

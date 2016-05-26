#ifndef ZM_STREAM_H
#define ZM_STREAM_H

#include "zmFeedConsumer.h"
#include "../libgen/libgenComms.h"
#include "../libgen/libgenBuffer.h"

#include <deque>

typedef std::deque<ByteBuffer *> PacketQueue;

class Connection;
class Encoder;

///
/// Class representing a generic network stream.
///
class Stream : public VideoConsumer
{
protected:
    Connection      *mConnection;   ///< Pointer to the network connection
    FeedProvider    *mProvider;     ///< Pointer to the frame provider supply the source frames
    Encoder         *mEncoder;      ///< Pointer to the encoder supplying the encoded data

protected:
    //bool writeFrame( Select::CommsList &writeable, const FeedFrame *frame );

public:
    Stream( const std::string &tag, const std::string &id, Connection *connection, FeedProvider *provider );
    Stream( const std::string &tag, const std::string &id, Connection *connection, Encoder *encoder );
    ~Stream();
};

#endif // ZM_STREAM_H

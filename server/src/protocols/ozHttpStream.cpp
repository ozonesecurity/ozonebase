#include "../base/oz.h"
#include "ozHttpStream.h"

#include "ozHttp.h"
#include "ozHttpSession.h"
#include "../base/ozConnection.h"
#include "../base/ozFeedFrame.h"
#include "../encoders/ozJpegEncoder.h"

/**
* @brief 
*
* @param tag
* @param httpSession
* @param connection
* @param provider
*/
HttpStream::HttpStream( const std::string &tag, HttpSession *httpSession, Connection *connection, FeedProvider *provider ) :
    Stream( tag, stringtf( "%X", httpSession->session() ), connection, provider ),
    Thread( identity() ),
    mHttpSession( httpSession )
{
    Debug( 2, "New HTTP stream" );
}

/**
* @brief 
*/
HttpStream::~HttpStream()
{
}

/**
* @brief 
*
* @return 
*/
int HttpStream::run()
{
    Select select( 60 );
    select.addWriter( mConnection->socket() );

    if ( !waitForProviders() )
        return( 1 );

    while ( !mStop && select.wait() >= 0 )
    {
        if ( mStop )
           break;
        Select::CommsList writeable = select.getWriteable();
        if ( writeable.size() <= 0 )
        {
            Error( "Writer timed out" );
            mStop = true;
            break;
        }
        mQueueMutex.lock();
        if ( !mFrameQueue.empty() )
        {
            for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
            {
                sendFrame( writeable, *iter );
                //delete *iter;
            }
            mFrameQueue.clear();
        }
        mQueueMutex.unlock();
        usleep( INTERFRAME_TIMEOUT );
    }

    return( 0 );
}

/**
* @brief 
*
* @param httpSession
* @param connection
* @param provider
* @param width
* @param height
* @param frameRate
* @param quality
*/
HttpImageStream::HttpImageStream( HttpSession *httpSession, Connection *connection, FeedProvider *provider, uint16_t width, uint16_t height, FrameRate frameRate, uint8_t quality ) :
    HttpStream( cClass(), httpSession, connection, provider )
{
    Debug( 2, "New HTTP image stream %d x %d @ %.2f (%d)", width, height, (double)frameRate, quality );
    std::string encoderKey = JpegEncoder::getPoolKey( provider->identity(), width, height, frameRate, quality );
    if ( !(mEncoder = Encoder::getPooledEncoder( encoderKey )) )
    {
        JpegEncoder *jpegEncoder = new JpegEncoder( provider->identity(), width, height, frameRate, quality );
        jpegEncoder->registerProvider( *provider );
        Encoder::poolEncoder( jpegEncoder );
        jpegEncoder->start();
        mEncoder = jpegEncoder;
    }
    registerProvider( *mEncoder );
}

/**
* @brief 
*/
HttpImageStream::~HttpImageStream()
{
    deregisterProvider( *mEncoder );
}

/**
* @brief 
*
* @param writeable
* @param frame
*
* @return 
*/
bool HttpImageStream::sendFrame( Select::CommsList &writeable, const FramePtr &frame )
{
    const ByteBuffer &packet = frame->buffer();

    std::string txHeaders = "--imgboundary\r\n";
    txHeaders += "Content-Type: image/jpeg\r\n";
    txHeaders += stringtf( "Content-Length: %zd\r\n\r\n", packet.size() );

    ByteBuffer txBuffer;
    txBuffer.reserve( packet.size()+128 );
    txBuffer.append( txHeaders.c_str(), txHeaders.size() );
    txBuffer.append( packet.data(), packet.size() );
    txBuffer.append( "\r\n", 2 );

    for ( Select::CommsList::iterator iter = writeable.begin(); iter != writeable.end(); iter++ )
    {
        if ( TcpInetSocket *socket = dynamic_cast<TcpInetSocket *>(*iter) )
        {
            if ( socket == mConnection->socket() )
            {
                int nBytes = socket->write( txBuffer.data(), txBuffer.size() );
                const FeedFrame *sourceFrame = frame->sourceFrame();
                Debug( 4, "Wrote %d bytes on sd %d, frame %ju<-%ju", nBytes, socket->getWriteDesc(), frame->id(), sourceFrame->id() );
                if ( nBytes != txBuffer.size() )
                {
                    Error( "Incomplete write, %d bytes instead of %zd", nBytes, packet.size() );
                    mStop = true;
                    return( false );
                }
            }
        }
    }
    return( true );
}

/**
* @brief 
*
* @param httpSession
* @param connection
* @param provider
*/
HttpDataStream::HttpDataStream( HttpSession *httpSession, Connection *connection, FeedProvider *provider ) :
    HttpStream( cClass(), httpSession, connection, provider )
{
    Debug( 2, "New HTTP data stream" );
    registerProvider( *provider );
}

/**
* @brief 
*/
HttpDataStream::~HttpDataStream()
{
    deregisterProvider( *mProvider );
}

/**
* @brief 
*
* @param writeable
* @param frame
*
* @return 
*/
bool HttpDataStream::sendFrame( Select::CommsList &writeable, const FramePtr &frame )
{
    const ByteBuffer &packet = frame->buffer();

    for ( Select::CommsList::iterator iter = writeable.begin(); iter != writeable.end(); iter++ )
    {
        if ( TcpInetSocket *socket = dynamic_cast<TcpInetSocket *>(*iter) )
        {
            if ( socket == mConnection->socket() )
            {
                int nBytes = socket->write( packet.data(), packet.size() );
                const FeedFrame *sourceFrame = frame->sourceFrame();
                Debug( 4, "Wrote %d bytes on sd %d, frame %ju<-%ju", nBytes, socket->getWriteDesc(), frame->id(), sourceFrame->id() );
                if ( nBytes != packet.size() )
                {
                    Error( "Incomplete write, %d bytes instead of %zd", nBytes, packet.size() );
                    mStop = true;
                    return( false );
                }
            }
        }
    }
    return( true );
}

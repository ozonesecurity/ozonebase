#include "zm.h"
#include "zmEncoder.h"

Mutex               Encoder::smPoolMutex;
Encoder::EncoderMap Encoder::smEncoderPool;

Encoder::Encoder( const std::string &cl4ss, const std::string &name ) :
    VideoConsumer( cl4ss, name ),
    VideoProvider( cl4ss, name ),
    mCodecContext( NULL ),
    mPooled( false ),
    mPoolKey( name ),
    mLastUse( time(NULL) )
{
}

Encoder::~Encoder()
{
}

void Encoder::cleanup()
{
    unpoolEncoder( this );
    if ( mCodecContext )
    {
        avcodec_close( mCodecContext );
        av_freep( &mCodecContext );
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
}

bool Encoder::queueFrame( FramePtr frame, FeedProvider *provider )
{
    if ( mPooled && ((time( NULL ) - mLastUse) > POOL_TIMEOUT) )
    {
        Debug( 2, "Expiring encoder %s from pool", mPoolKey.c_str() );
        poolingExpired();
        return( false );
    }
    return( FeedConsumer::queueFrame( frame, provider ) );
}

Encoder *Encoder::getPooledEncoder( const std::string &poolKey )
{
    smPoolMutex.lock();
    EncoderMap::iterator iter = smEncoderPool.find( poolKey );
    Encoder *result = ( iter != smEncoderPool.end() ) ? iter->second : NULL;
    smPoolMutex.unlock();
    return( result );
}

bool Encoder::poolEncoder( Encoder *encoder )
{
    Debug( 2, "Adding encoder %s to pool", encoder->mPoolKey.c_str() );
    smPoolMutex.lock();
    std::pair<EncoderMap::iterator,bool> result = smEncoderPool.insert( EncoderMap::value_type( encoder->mPoolKey, encoder ) );
    smPoolMutex.unlock();
    encoder->mPooled = true;
    return( result.second );
}

bool Encoder::unpoolEncoder( Encoder *encoder )
{
    Debug( 2, "Removing encoder %s from pool", encoder->mPoolKey.c_str() );
    smPoolMutex.lock();
    size_t result = smEncoderPool.erase( encoder->mPoolKey );
    smPoolMutex.unlock();
    encoder->mPooled = false;
    return( result > 0 );
}

/*
uint16_t Encoder::width() const
{
    return( mCodecContext->width );
}

uint16_t Encoder::height() const
{
    return( mCodecContext->height );
}

int Encoder::frameRate() const
{
    return( mCodecContext->time_base.den/mCodecContext->time_base.num );
}

int Encoder::bitRate() const
{
    return( mCodecContext->bit_rate );
}
*/

#include "oz.h"
#include "ozEncoder.h"

Mutex               Encoder::smPoolMutex;
Encoder::EncoderMap Encoder::smEncoderPool;

/**
* @brief 
*
* @param cl4ss
* @param name
*/
Encoder::Encoder( const std::string &cl4ss, const std::string &name ) :
    VideoConsumer( cl4ss, name ),
    VideoProvider( cl4ss, name ),
    mCodecContext( NULL ),
    mPooled( false ),
    mPoolKey( name ),
    mLastUse( time(NULL) )
{
}

/**
* @brief 
*/
Encoder::~Encoder()
{
}

/**
* @brief 
*/
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

/**
* @brief 
*
* @param frame
* @param provider
*
* @return 
*/
bool Encoder::queueFrame( const FramePtr &frame, FeedProvider *provider )
{
    if ( mPooled && ((time( NULL ) - mLastUse) > POOL_TIMEOUT) )
    {
        Debug( 2, "Expiring encoder %s from pool", mPoolKey.c_str() );
        poolingExpired();
        return( false );
    }
    return( FeedConsumer::queueFrame( frame, provider ) );
}

/**
* @brief 
*
* @param poolKey
*
* @return 
*/
Encoder *Encoder::getPooledEncoder( const std::string &poolKey )
{
    smPoolMutex.lock();
    EncoderMap::iterator iter = smEncoderPool.find( poolKey );
    Encoder *result = ( iter != smEncoderPool.end() ) ? iter->second : NULL;
    smPoolMutex.unlock();
    return( result );
}

/**
* @brief 
*
* @param encoder
*
* @return 
*/
bool Encoder::poolEncoder( Encoder *encoder )
{
    Debug( 2, "Adding encoder %s to pool", encoder->mPoolKey.c_str() );
    smPoolMutex.lock();
    std::pair<EncoderMap::iterator,bool> result = smEncoderPool.insert( EncoderMap::value_type( encoder->mPoolKey, encoder ) );
    smPoolMutex.unlock();
    encoder->mPooled = true;
    return( result.second );
}

/**
* @brief 
*
* @param encoder
*
* @return 
*/
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

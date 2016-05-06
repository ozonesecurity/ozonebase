#include "zm.h"
#include "zmFeedProvider.h"

#include "zmFeedConsumer.h"
#include "zmFeedFrame.h"

Mutex                       FeedProvider::smProviderMutex;
FeedProvider::ProviderMap   FeedProvider::smProviders;

FeedProvider *FeedProvider::find( const std::string &identity )
{
    smProviderMutex.lock();
    ProviderMap::iterator iter = smProviders.find( identity );
    FeedProvider *provider = ( iter != smProviders.end() ) ? iter->second : NULL;
    smProviderMutex.unlock();
    return( provider );
}

/*
bool FeedProvider::verify( const std::string &identity )
{
    smProviderMutex.lock();
    ProviderMap::iterator iter = smProviders.find( identity );
    bool result = ( iter != smProviders.end() );
    smProviderMutex.unlock();
    if ( result )
    {
        Debug( 2, "Verified provider %s", identity.c_str() );
    }
    else
    {
        Warning( "Unable to verify provider %s", identity.c_str() );
    }
    return( result );
}
*/

FeedProvider::FeedProvider() :
    mState( PROV_WAIT ),
    mFrameCount( 0 ),
    mCanPoll( true ),
    mLastFrame( 0 ),
    mHasVideo( false ),
    mHasAudio( false )
{
}

void FeedProvider::addToMap()
{
    smProviderMutex.lock();
    std::pair<ProviderMap::iterator,bool> result = smProviders.insert( ProviderMap::value_type( identity(), this ) );
    if ( !result.second )
        Fatal( "Unable to add duplicate provider %s to map", cidentity() )
    smProviderMutex.unlock();
}

void FeedProvider::removeFromMap()
{
    smProviderMutex.lock();
    if ( smProviders.erase( identity() ) != 1 )
        Fatal( "Unable to remove provider %s from map, not found", cidentity() )
    smProviderMutex.unlock();
}

void FeedProvider::cleanup()
{
    setEnded();
    removeFromMap();
    mConsumerMutex.lock();
    for ( ConsumerMap::iterator iter = mConsumers.begin(); iter != mConsumers.end(); iter++ )
        if ( iter->first->hasProvider( *this ) )
            iter->first->deregisterProvider( *this, false );
    mConsumers.clear();
    mConsumerMutex.unlock();
}

bool FeedProvider::registerConsumer( FeedConsumer &consumer, const FeedLink &link )
{
    Debug( 2, "%s - Registering consumer %s", cidentity(), consumer.cidentity() );
    if ( !mCanPoll && link.isPolled() )
        Fatal( "%s - Attempt to register consumer %s with polled link, not supported for this provider", cidentity(), consumer.cidentity() );
    mConsumerMutex.lock();
    std::pair<ConsumerMap::iterator,bool> result = mConsumers.insert( ConsumerMap::value_type( &consumer, link ) );
    mConsumerMutex.unlock();
    if ( !consumer.hasProvider( *this ) )
       consumer.registerProvider( *this );
    return( result.second );
}

bool FeedProvider::deregisterConsumer( FeedConsumer &consumer, bool reciprocate )
{
    Debug( 2, "%s - Deregistering consumer %s", cidentity(), consumer.cidentity() );
    mConsumerMutex.lock();
    bool result = ( mConsumers.erase( &consumer ) == 1 );
    mConsumerMutex.unlock();
    if ( reciprocate && consumer.hasProvider( *this ) )
       consumer.deregisterProvider( *this );
    return( result );
}

bool FeedProvider::hasConsumer() const
{
    mConsumerMutex.lock();
    bool result = !mConsumers.empty();
    mConsumerMutex.unlock();
    return( result );
}

bool FeedProvider::hasConsumer( FeedConsumer &consumer ) const
{
    mConsumerMutex.lock();
    ConsumerMap::const_iterator iter = mConsumers.find( &consumer );
    bool result = ( iter != mConsumers.end() );
    mConsumerMutex.unlock();
    return( result );
}

void FeedProvider::distributeFrame( FramePtr frame )
{
    setReady();
    mConsumerMutex.lock();
    ConsumerList badConsumers;
    for ( ConsumerMap::iterator iter = mConsumers.begin(); iter != mConsumers.end(); iter++ )
    {
        if ( iter->second.isQueued() )
            if ( !iter->second.hasComparators() || iter->second.compare( frame, iter->first ) )
                if ( !iter->first->queueFrame( frame, this ) )
                    badConsumers.push_back( iter->first );
    }
    mConsumerMutex.unlock();
    for ( ConsumerList::iterator iter = badConsumers.begin(); iter != badConsumers.end(); iter++ )
        deregisterConsumer( *(*iter) );
    if ( mCanPoll )
    {
        mFrameMutex.lock();
        mLastFrame = frame;
        mFrameMutex.unlock();
    }
}

FramePtr FeedProvider::pollFrame() const
{
    if ( !ready() || !mCanPoll )
        return( FramePtr( NULL ) );
    mFrameMutex.lock();
    FramePtr frame( mLastFrame );
    mFrameMutex.unlock();
    return( frame );
}

VideoProvider::VideoProvider( const std::string &cl4ss, const std::string &name )
{
    mHasVideo = true;
    setIdentity( cl4ss, name );
    addToMap();
    //Info( "VideoProvider : %s + %s", mTag.c_str(), mId.c_str() );
}

AudioProvider::AudioProvider( const std::string &cl4ss, const std::string &name )
{
    mHasAudio = true;
    setIdentity( cl4ss, name );
    addToMap();
    //Info( "AudioProvider : %s + %s", mTag.c_str(), mId.c_str() );
}

AudioVideoProvider::AudioVideoProvider( const std::string &cl4ss, const std::string &name )
{
    setIdentity( cl4ss, name );
    addToMap();
    //Info( "AudioVideoProvider : %s + %s", mTag.c_str(), mId.c_str() );
}

bool AudioVideoProvider::videoFramesOnly( FramePtr frame, const FeedConsumer * )
{
    const VideoFrame *videoFrame = dynamic_cast<const VideoFrame *>(frame.get());
    return( videoFrame != NULL );
}

bool AudioVideoProvider::audioFramesOnly( FramePtr frame, const FeedConsumer * )
{
    const AudioFrame *audioFrame = dynamic_cast<const AudioFrame *>(frame.get());
    return( audioFrame != NULL );
}

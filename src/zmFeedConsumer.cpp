#include "zm.h"
#include "zmFeedConsumer.h"
#include "zmFeedProvider.h"

void FeedConsumer::cleanup()
{
    mProviderMutex.lock();
    for ( ProviderMap::iterator iter = mProviders.begin(); iter != mProviders.end(); iter++ )
        if ( iter->first->hasConsumer( *this ) )
            iter->first->deregisterConsumer( *this, false );
    mProviders.clear();
    mProviderMutex.unlock();
    mQueueMutex.lock();
    mFrameQueue.clear();
    mQueueMutex.unlock();
}

bool FeedConsumer::registerProvider( FeedProvider &provider, const FeedLink &link )
{
    Debug( 2, "%s - Registering provider %s", cidentity(), provider.cidentity() );
    mProviderMutex.lock();
    if ( mProviders.size() >= mProviderLimit )
        Fatal( "%s - Can't register provider above limit of %d", cidentity(), mProviderLimit );
    std::pair<ProviderMap::iterator,bool> result = mProviders.insert( ProviderMap::value_type( &provider, link ) );
    mProviderMutex.unlock();
    if ( result.second )
    {
        if ( !provider.hasConsumer( *this ) )
            provider.registerConsumer( *this, link );
    }
    return( result.second );
}

bool FeedConsumer::deregisterProvider( FeedProvider &provider, bool reciprocate )
{
    Debug( 2, "%s - Deregistering provider %s", cidentity(), provider.cidentity() );
    mProviderMutex.lock();
    bool result = ( mProviders.erase( &provider ) == 1 );
    mProviderMutex.unlock();
    if ( reciprocate && provider.hasConsumer( *this ) )
        provider.deregisterConsumer( *this );
    return( result );
}

bool FeedConsumer::hasProvider() const
{
    mProviderMutex.lock();
    bool result = !mProviders.empty();
    mProviderMutex.unlock();
    return( result );
}

bool FeedConsumer::hasProvider( FeedProvider &provider ) const
{
    mProviderMutex.lock();
    ProviderMap::const_iterator iter = mProviders.find( &provider );
    bool result = ( iter != mProviders.end() );
    mProviderMutex.unlock();
    return( result );
}

bool FeedConsumer::waitForProviders()
{
    mProviderMutex.lock();
    if ( mProviders.empty() )
    {
        Warning( "%s: No providers registered for consumer", cidentity() );
    }
    mProviderMutex.unlock();
    int waitCount, readyCount, badCount;
    do
    {
        waitCount = 0;
        readyCount = 0;
        badCount = 0;
        mProviderMutex.lock();
        Info( "%s: Got %d providers", cidentity(), mProviders.size() );
        for ( ProviderMap::const_iterator iter = mProviders.begin(); iter != mProviders.end(); iter++ )
        {
            if ( iter->first->ready() )
            {
                readyCount++;
            }
            else
            {
                if ( iter->first->wait() )
                {
                    Info( "%s: Provider %s not ready", cidentity(), iter->first->cidentity() );
                    waitCount++;
                }
                else
                {
                    if ( iter->first->ended() )
                    {
                        Error( "%s: Provider %s has ended", cidentity(), iter->first->cidentity() );
                    }
                    else if ( iter->first->error() )
                    {
                        Error( "%s: Provider %s has failed", cidentity(), iter->first->cidentity() );
                    }
                    badCount++;
                }
            }
        }
        mProviderMutex.unlock();
        if ( waitCount > 0 && badCount == 0 )
        {
            mQueueMutex.lock();
            mFrameQueue.clear();
            mQueueMutex.unlock();
            usleep( 10000 );
        }
    } while( waitCount > 0 && badCount == 0 );
    //Info( "%s: Returning %d", cidentity(), readyCount > 0 && badCount == 0 );
    return( readyCount > 0 && badCount == 0 );
}

bool FeedConsumer::checkProviders()
{
    mProviderMutex.lock();
    bool result = !mProviders.empty();
    for ( ProviderMap::const_iterator iter = mProviders.begin(); iter != mProviders.end(); iter++ )
    {
        if ( iter->first->ready() )
        {
        }
        else if ( !iter->first->ended() )
        {
            Info( "%s: Provider %s has ended", cidentity(), iter->first->cidentity() );
        }
        else
        {
            if ( iter->first->wait() )
            {
                Error( "%s: Provider %s not ready", cidentity(), iter->first->cidentity() );
            }
            else if ( !iter->first->error() )
            {
                Error( "%s: Provider %s has failed", cidentity(), iter->first->cidentity() );
            }
            else
            {
                Fatal( "%s: Provider %s has unexpected state", cidentity(), iter->first->cidentity() );
            }
            result = false;
        }
    }
    mProviderMutex.unlock();
    if ( !result )
    {
        Thread *thread = dynamic_cast<Thread *>( this );
        if ( thread )
            thread->stop();
    }
    return( result );
}

bool FeedConsumer::queueFrame( FramePtr frame, FeedProvider *provider )
{
    bool result = true;
    mQueueMutex.lock();
    if ( mFrameQueue.size() > MAX_QUEUE_SIZE )
    {
        Error( "%s: Maximum queue size exceeded, got %d frames, not running?", cidentity(), mFrameQueue.size() );
        result = false;
    }
    else
    {
        mFrameQueue.push_back( frame );
    }
    mQueueMutex.unlock();
    return( result );
}

VideoConsumer::VideoConsumer( const std::string &cl4ss, const std::string &name, int providerLimit ) :
    FeedConsumer( providerLimit )
{
     setIdentity( cl4ss, name );
}

VideoConsumer::VideoConsumer( const std::string &cl4ss, FeedProvider &provider, const FeedLink &link ) :
    FeedConsumer( provider, link )
{
     setIdentity( cl4ss, provider.name() );
}

VideoConsumer::VideoConsumer()
{
}

bool VideoConsumer::registerProvider( FeedProvider &provider, const FeedLink &link  )
{
    return( FeedConsumer::registerProvider( provider, link ) );
}

VideoProvider *VideoConsumer::videoProvider() const
{
    return( mProviders.empty() ? NULL : dynamic_cast<VideoProvider *>(mProviders.begin()->first) );
}

AudioConsumer::AudioConsumer( const std::string &cl4ss, const std::string &name, int providerLimit ) :
    FeedConsumer( providerLimit )
{
     setIdentity( cl4ss, name );
}

AudioConsumer::AudioConsumer( const std::string &cl4ss, FeedProvider &provider, const FeedLink &link ) :
    FeedConsumer( provider, link )
{
     setIdentity( cl4ss, provider.name() );
}

AudioConsumer::AudioConsumer()
{
}

bool AudioConsumer::registerProvider( FeedProvider &provider, const FeedLink &link  )
{
    return( FeedConsumer::registerProvider( provider, link ) );
}

AudioProvider *AudioConsumer::audioProvider() const
{
    return( mProviders.empty() ? NULL : dynamic_cast<AudioProvider *>(mProviders.begin()->first) );
}

#include "oz.h"
#include "ozFeedConsumer.h"
#include "ozFeedProvider.h"

/**
* @brief 
*/
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
/**
* @brief 
*
* @return 
*/
bool FeedConsumer::deregisterAllProviders()
{
    cleanup();
}

/**
* @brief 
*
* @param provider
* @param link
*
* @return 
*/
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

/**
* @brief 
*
* @param provider
* @param reciprocate
*
* @return 
*/
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

/**
* @brief 
*
* @return 
*/
bool FeedConsumer::hasProvider() const
{
    mProviderMutex.lock();
    bool result = !mProviders.empty();
    mProviderMutex.unlock();
    return( result );
}

/**
* @brief 
*
* @param provider
*
* @return 
*/
bool FeedConsumer::hasProvider( FeedProvider &provider ) const
{
    mProviderMutex.lock();
    ProviderMap::const_iterator iter = mProviders.find( &provider );
    bool result = ( iter != mProviders.end() );
    mProviderMutex.unlock();
    return( result );
}

/**
* @brief 
*
* @return 
*/
bool FeedConsumer::waitForProviders( unsigned int count )
{
    int waitFor = count ? count : mProviders.size();
    int maxWaitCount = mProviders.size() - waitFor;

    mProviderMutex.lock();
    if ( mProviders.empty() )
    {
        Debug( 5,"%s: No providers registered for consumer", cidentity() );
    }
    mProviderMutex.unlock();
    int waitCount, readyCount, badCount;
    do
    {
        waitCount = 0;
        readyCount = 0;
        badCount = 0;
        mProviderMutex.lock();
        Debug( 5,"%s: Got %lu providers", cidentity(), mProviders.size() );
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
                    Debug(1, "%s: Provider %s not ready", cidentity(), iter->first->cidentity() );
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
        if ( waitCount > maxWaitCount && badCount == 0 )
        {
            mQueueMutex.lock();
            mFrameQueue.clear();
            mQueueMutex.unlock();
            usleep( INTERFRAME_TIMEOUT );
        }
    } while( waitCount > maxWaitCount && badCount == 0 );
    //Info( "%s: Returning %d", cidentity(), readyCount > 0 && badCount == 0 );
    return( readyCount > 0 && badCount == 0 );
}

/**
* @brief 
*
* @return 
*/
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

/**
* @brief 
*
* @param frame
* @param provider
*
* @return 
*/
bool FeedConsumer::queueFrame( const FramePtr &frame, FeedProvider *provider )
{
    bool result = true;
    mQueueMutex.lock();
    if ( mFrameQueue.size() >= MAX_QUEUE_SIZE )
    {
        Error( "%s: Maximum queue size exceeded, got %lu frames, not running?", cidentity(), mFrameQueue.size() );
        result = false;
    }
    else
    {
        mFrameQueue.push_back( frame );
    }
    mQueueMutex.unlock();
    return( result );
}

/**
* @brief 
*
* @param cl4ss
* @param name
* @param providerLimit
*/
DataConsumer::DataConsumer( const std::string &cl4ss, const std::string &name, int providerLimit ) :
    FeedConsumer( providerLimit )
{
     setIdentity( cl4ss, name );
}

/**
* @brief 
*
* @param cl4ss
* @param provider
* @param link
*/
DataConsumer::DataConsumer( const std::string &cl4ss, FeedProvider &provider, const FeedLink &link ) :
    FeedConsumer( provider, link )
{
     setIdentity( cl4ss, provider.name() );
}

/**
* @brief 
*/
DataConsumer::DataConsumer()
{
}

/**
* @brief 
*
* @return 
*/
DataProvider *DataConsumer::dataProvider() const
{
    return( mProviders.empty() ? NULL : dynamic_cast<DataProvider *>(mProviders.begin()->first) );
}

/**
* @brief 
*
* @param cl4ss
* @param name
* @param providerLimit
*/
VideoConsumer::VideoConsumer( const std::string &cl4ss, const std::string &name, int providerLimit ) :
    FeedConsumer( providerLimit )
{
     setIdentity( cl4ss, name );
}

/**
* @brief 
*
* @param cl4ss
* @param provider
* @param link
*/
VideoConsumer::VideoConsumer( const std::string &cl4ss, FeedProvider &provider, const FeedLink &link ) :
    FeedConsumer( provider, link )
{
     setIdentity( cl4ss, provider.name() );
}

/**
* @brief 
*/
VideoConsumer::VideoConsumer()
{
}

/**
* @brief 
*
* @return 
*/
VideoProvider *VideoConsumer::videoProvider() const
{
    VideoProvider *provider = mProviders.empty() ? NULL : dynamic_cast<VideoProvider *>(mProviders.begin()->first);
    if ( !provider )
        throw Exception( "No video provider found" );
    return( provider );
}

/**
* @brief 
*
* @param cl4ss
* @param name
* @param providerLimit
*/
AudioConsumer::AudioConsumer( const std::string &cl4ss, const std::string &name, int providerLimit ) :
    FeedConsumer( providerLimit )
{
     setIdentity( cl4ss, name );
}

/**
* @brief 
*
* @param cl4ss
* @param provider
* @param link
*/
AudioConsumer::AudioConsumer( const std::string &cl4ss, FeedProvider &provider, const FeedLink &link ) :
    FeedConsumer( provider, link )
{
     setIdentity( cl4ss, provider.name() );
}

/**
* @brief 
*/
AudioConsumer::AudioConsumer()
{
}

/**
* @brief 
*
* @return 
*/
AudioProvider *AudioConsumer::audioProvider() const
{
    return( mProviders.empty() ? NULL : dynamic_cast<AudioProvider *>(mProviders.begin()->first) );
}

/**
* @brief 
*
* @param cl4ss
* @param name
* @param providerLimit
*/
GeneralConsumer::GeneralConsumer( const std::string &cl4ss, const std::string &name, int providerLimit ) :
    FeedConsumer( providerLimit )
{
     setIdentity( cl4ss, name );
}

/**
* @brief 
*
* @param cl4ss
* @param provider
* @param link
*/
GeneralConsumer::GeneralConsumer( const std::string &cl4ss, FeedProvider &provider, const FeedLink &link ) :
    FeedConsumer( provider, link )
{
     setIdentity( cl4ss, provider.name() );
}

/**
* @brief 
*/
GeneralConsumer::GeneralConsumer()
{
}

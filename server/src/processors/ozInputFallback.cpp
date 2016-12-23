#include "../base/oz.h"
#include "ozInputFallback.h"

#include "../base/ozFeedFrame.h"

/**
* @brief 
*
* @param name
* @param timeout
*/
InputFallback::InputFallback( const std::string &name, double timeout ) :
    VideoConsumer( cClass(), name, 2 ),
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mProviderList( mProviderLimit ),
    mCurrentProvider( NULL ),
    mProviderTimeout( timeout )
{
    for ( int i = 0; i < mProviderLimit; i++ )
        mProviderList[i] = NULL;
}

/**
* @brief 
*/
InputFallback::~InputFallback()
{
}

/**
* @brief 
*
* @param provider
* @param link
*
* @return 
*/
bool InputFallback::registerProvider( FeedProvider &provider, const FeedLink &link )
{
    for ( unsigned int i = 0; i < mProviderList.size(); i++ )
    {
        if ( !mProviderList[i] )
        {
            if ( VideoConsumer::registerProvider( provider, link ) )
            {
                mProviderList[i] = &provider;
                ProviderStats *providerStats = new ProviderStats;
                providerStats->priority = i;
                providerStats->frameCount = 0;
                providerStats->lastFrameTime = time64();
                mProviderStats[&provider] = providerStats;
                return( true );
            }
        }
    }
    return( false );
}

/**
* @brief 
*
* @param provider
* @param reciprocate
*
* @return 
*/
bool InputFallback::deregisterProvider( FeedProvider &provider, bool reciprocate )
{
    for ( unsigned int i = 0; i < mProviderList.size(); i++ )
    {
        if ( mProviderList[i] == &provider )
        {
            mProviderStats.erase( mProviderList[i] );
            mProviderList[i] = NULL;
            break;
        }
    }
    return( VideoConsumer::deregisterProvider( provider, reciprocate ) );
}

/**
* @brief 
*
* @return 
*/
int InputFallback::run()
{
    // Wait for video source to be ready
    if ( waitForProviders( 1 ) )
    {
        ProviderStats *providerStats = NULL;
        mCurrentProvider = dynamic_cast<const VideoProvider *>( mProviderList[0] );
        while ( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                Debug( 3, "Got %zd frames on queue", mFrameQueue.size() );
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    const FeedFrame *frame = iter->get();
                    const FeedProvider *provider = frame->provider();
                    providerStats = mProviderStats[provider];
                    providerStats->frameCount++;
                    providerStats->lastFrameTime = time64();
                    if ( providerStats->priority < mProviderStats[mCurrentProvider]->priority )
                    {
                        Debug( 1, "Higher priority provider available, switching to '%s'", provider->cname() );
                        mCurrentProvider = dynamic_cast<const VideoProvider *>( provider );
                    }
                    if ( provider == mCurrentProvider )
                    {
                        // Can probably just distribute original frame, depends if we want trail of processors
                        //VideoFrame *videoFrame = new VideoFrame( this, *iter );
                        //distributeFrame( FramePtr( videoFrame ) );
                        distributeFrame( *iter );
                    }
                    mFrameCount++;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            providerStats = mProviderStats[mCurrentProvider];
            if ( (time64() - providerStats->lastFrameTime) > (mProviderTimeout * 1000000LL) )
            {
                unsigned int i;
                for ( i = providerStats->priority+1; i < mProviderList.size(); i++ )
                {
                    mCurrentProvider = dynamic_cast<const VideoProvider *>( mProviderList[i] );
                    Debug( 1, "Current provider timed out, switching to '%s'", mCurrentProvider->cname() );
                    break;
                }
                if ( i == mProviderList.size() )
                    Error( "Current provider timed out, no fallback available" );
            }
            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}

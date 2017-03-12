#ifndef OZ_FEED_CONSUMER_H
#define OZ_FEED_CONSUMER_H

#include "ozFeedBase.h"
#include "ozFeedFrame.h"
#include "../libgen/libgenThread.h"
#include <map>
#include <deque>

class FeedProvider;

///
/// Abstract base class for consumers of FeedFrame objects as supplied by FeedProvider objects. Frames are normally
/// placed on the frame queue directly by providers but consumers can also access the last frame directly by a polled interface.
///
class FeedConsumer : virtual public FeedBase
{
private:
    static const int MAX_QUEUE_SIZE = 10000;    ///< Limit to thew number of frames on a consumers queue. Any more than this and
                                                ///< the consumer is removed by the provider to avoid memory exhaustion. Usually
                                                ///< smaller than this but enlarged for testing.

public:
    typedef std::map<FeedProvider *,FeedLink>   ProviderMap;
    typedef std::deque<FramePtr>                FrameQueue;

protected:
    Mutex           mQueueMutex;                ///< Protects the frame queue
    FrameQueue      mFrameQueue;                ///< Queue of frames from providers that are ready to be handled
    int             mProviderLimit;             ///< Max number of providers, many consumers can only have one
    mutable Mutex   mProviderMutex;             ///< Protects the provider map
    ProviderMap     mProviders;                 ///< Collection of providers

protected:
    FeedConsumer( int providerLimit=1 ) :
        mProviderLimit( providerLimit )
    {
    }
    FeedConsumer( FeedProvider &provider, const FeedLink &link=gQueuedFeedLink ) :
        mProviderLimit( 1 )
    {
        registerProvider( provider, link );
    }

    bool waitForProviders( unsigned int=0 );    ///< Return when all providers are ready to supply frames. Returns false
                                                ///< if problems occur such as providers going away.
    bool checkProviders();                      ///< Verify that providers exist and are in good condition
    virtual void cleanup();                     ///< Tidy up relationships with providers etc before destruction.

public:
    virtual ~FeedConsumer()
    {
        cleanup();
    }

    ///
    /// Register a provider to this consumer. If comparator is null all frames are passed on
    /// otherwise only those for whom the comparator function returns true. Returns true for
    /// success, false for failure.
    ///
    virtual bool registerProvider( FeedProvider &provider, const FeedLink &link=gQueuedFeedLink  );

    ///
    /// Deregister a provider from this consumer. If reciprocate is true then also
    /// deregister this provider from the consumer. Returns true for success, false for failure.
    ///
    virtual bool deregisterProvider( FeedProvider &provider, bool reciprocate=true );


    ///
    /// Deregisters all providers (and reciprocal consumers in providers)
    ///
    virtual bool deregisterAllProviders();

    /// Return the first provider, shortcut for when providers == 1 
    FeedProvider *provider() const { return( mProviders.empty() ? NULL : mProviders.begin()->first ); }

    ///
    /// Returns true if this consumer has any providers.
    ///
    virtual bool hasProvider() const;

    ///
    /// Returns true if the given provider is registered to this consumer.
    ///
    virtual bool hasProvider( FeedProvider &provider ) const;

    ///
    /// Place the given frame onto this consumers frame queue, the supplying provider is also given. Used by
    /// providers for distributing frames.
    ///
    virtual bool queueFrame( const FramePtr &, FeedProvider * );
    //virtual bool writeFrame( const FeedFrame * )=0;
};

class DataProvider;

///
/// Specialised base class for video consumers
///
class DataConsumer : public FeedConsumer
{
protected:
    DataConsumer( const std::string &tag, const std::string &id, int providerLimit=1 );
    DataConsumer( const std::string &tag, FeedProvider &provider, const FeedLink &link=gQueuedFeedLink );
    DataConsumer();

public:
    /// Return the first provider cast appropriately, shortcut for when providers == 1 
    DataProvider *dataProvider() const;
};

class VideoProvider;

///
/// Specialised base class for video consumers
///
class VideoConsumer : public FeedConsumer
{
protected:
    VideoConsumer( const std::string &tag, const std::string &id, int providerLimit=1 );
    VideoConsumer( const std::string &tag, FeedProvider &provider, const FeedLink &link=gQueuedFeedLink );
    VideoConsumer();

public:
    /// Return the first provider cast appropriately, shortcut for when providers == 1 
    VideoProvider *videoProvider() const;
};

class AudioProvider;

///
/// Specialised base class for video consumers
///
class AudioConsumer : public FeedConsumer
{
protected:
    AudioConsumer( const std::string &tag, const std::string &id, int providerLimit=1 );
    AudioConsumer( const std::string &tag, FeedProvider &provider, const FeedLink &link=gQueuedFeedLink );
    AudioConsumer();

public:
    /// Return the first provider cast appropriately, shortcut for when providers == 1 
    AudioProvider *audioProvider() const;
};

///
/// General base class for consumers of all frames
///
class GeneralConsumer : public FeedConsumer
{
protected:
	GeneralConsumer( const std::string &tag, const std::string &id, int providerLimit=1 );
	GeneralConsumer( const std::string &tag, FeedProvider &provider, const FeedLink &link=gQueuedFeedLink );
	GeneralConsumer();
};

#endif // OZ_FEED_CONSUMER_H

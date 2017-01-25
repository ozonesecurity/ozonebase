#ifndef OZ_FEED_PROVIDER_H
#define OZ_FEED_PROVIDER_H

#include "oz.h"
#include "ozFeedBase.h"
#include "ozFeedFrame.h"
#include "ozFfmpeg.h"
#include "../libgen/libgenThread.h"
#include <map>
#include <deque>
#include <stdint.h>

class FeedConsumer;

///
/// Base class for frame providers. That is components that either originate FeedFrame objects or forward already
/// existing ones onwards. Frames are either placed on a consumers frame queue, or occasionally requested directly
/// by the consumer. Providers can have an unlimited number of consumers but more consumers can only have one
/// provider.
///
class FeedProvider : virtual public FeedBase
{
protected:
    static const int RECONNECT_TIMEOUT = 1;         ///< How long to wait before reconnecting to a lost source, in seconds

protected:
    typedef std::deque<FeedConsumer *>              ConsumerList;
    typedef std::map<FeedConsumer *,FeedLink>       ConsumerMap;

private:
    typedef std::map<std::string,FeedProvider *>    ProviderMap;

public:
    typedef enum { PROV_ERROR=-1, PROV_WAIT, PROV_READY, PROV_ENDED }  State;

private:
    static Mutex        smProviderMutex;            ///< Protects the providers map from simultaneous access
    static ProviderMap  smProviders;                ///< Static map of providers, used for looking up existing providers

protected:
    State           mState;                         ///< Provider state
    uint64_t        mFrameCount;                    ///< Number of frames, advisory only
    bool            mCanPoll;                       ///< Whether consumers can request frames on demand
    mutable Mutex   mFrameMutex;                    ///< Protects the 'on-demand' frame
    FramePtr        mLastFrame;                     ///< The 'on-demand' frame
    mutable Mutex   mConsumerMutex;                 ///< Protects the consumers
    ConsumerMap     mConsumers;                     ///< Collection of frame consumer

protected:
    bool            mHasVideo;
    bool            mHasAudio;

protected:
    virtual void distributeFrame( const FramePtr &frame ); ///< Pass a frame on to all registered consumers

public:
    static FeedProvider *find( const std::string &identity );  ///< Locate a provider with the given identity
    //static bool verify( const std::string &identity );

public:
    static bool videoFramesOnly( const FramePtr &, const FeedConsumer * );  ///< Comparator function to allow consumers to select only video frames.
    static bool audioFramesOnly( const FramePtr &, const FeedConsumer * );  ///< Comparator function to allow consumers to select only audio frames.
    static bool dataFramesOnly( const FramePtr &, const FeedConsumer * );   ///< Comparator function to allow consumers to select only data frames.

public:
    static bool noFrames( const FramePtr &, const FeedConsumer * )  /// Comparator function that can be used to suppress frames
    {
        return( false );
    }

protected:
    FeedProvider();
    
    void addToMap();                                ///< Add this provider to the static provider map
    void removeFromMap();                           ///< Remove this provider from the static provider map
    virtual void cleanup();                         ///< Tidy up relations before destruction

    void setReady() { if ( mState == PROV_WAIT ) mState = PROV_READY; }
    void setEnded() { if ( mState >= PROV_WAIT ) mState = PROV_ENDED; }
    void setError() { mState = PROV_ERROR; }

public:
    virtual ~FeedProvider()
    {
        cleanup();
    }

    /// Provides video frames
    bool hasVideo() const { return( mHasVideo ); }
    /// Provides audio frames
    bool hasAudio() const { return( mHasAudio ); }

    ///
    /// Register a consumer to this provider. If comparator is null all frames are passed on
    /// otherwise only those for whom the comparator function returns true. Returns true for
    /// success, false for failure.
    ///
    virtual bool registerConsumer( FeedConsumer &consumer, const FeedLink &link=gQueuedFeedLink );

    ///
    /// Deregister a consumer from this provider. If reciprocate is true then also
    /// deregister this provider from the consumer. Returns true for success, false for failure.
    ///
    virtual bool deregisterConsumer( FeedConsumer &consumer, bool reciprocate=true );

    ///
    /// Returns true if this provider has any consumers.
    ///
    virtual bool hasConsumer() const;

    ///
    /// Returns true if the given consumer is registered to this provider.
    ///
    virtual bool hasConsumer( FeedConsumer &consumer ) const;

    /// Indicate whether this provider is ready to supply frames
    bool wait() const { return( mState == PROV_WAIT ); }
    bool ready() const { return( mState == PROV_READY ); }
    bool ended() const { return( mState == PROV_ENDED ); }
    bool error() const { return( mState == PROV_ERROR ); }

    /// Return true if there is a new frame available that the one supplied
    bool frameReady( uint64_t lastFrameCount ) const { return( mFrameCount>lastFrameCount ); }

    /// Return the number of frames this provider has supplied
    uint64_t frameCount() const { return( mFrameCount ); }

    /// Indicate whether this provider can supply frames by polling
    bool canPoll() const { return( mCanPoll ); }

    /// Enable the polled frame interface if supported
    void setPolling( bool poll ) { mCanPoll = poll; }

    /// Return a reference to the last generated frame
    FramePtr pollFrame() const;
};

///
/// Specialised abstract base class for providers of DataFrame objects
///
class DataProvider : virtual public FeedProvider
{
protected:
    DataProvider( const std::string &tag, const std::string &id );
    DataProvider() { }
    ~DataProvider() { }

public:
};

///
/// Specialised abstract base class for providers of VideoFrame objects
///
class VideoProvider : virtual public FeedProvider
{
protected:
    VideoProvider( const std::string &tag, const std::string &id );
    VideoProvider() { }
    ~VideoProvider() { }

public:
    virtual AVPixelFormat pixelFormat() const=0;    ///< Return the image format of video frames supplied by this provider
    virtual uint16_t width() const=0;               ///< Return the width (in pixels) of video frames supplied by this provider
    virtual uint16_t height() const=0;              ///< Return the height (in pixels) of video frames supplied by this provider
    virtual FrameRate frameRate() const=0;          ///< Return the frame rate at this this provider supplies frames. May not be available or accurate.
};

///
/// Specialised abstract base class for providers of AudioFrame objects
///
class AudioProvider : virtual public FeedProvider
{
protected:
    AudioProvider( const std::string &tag, const std::string &id );
    AudioProvider() { }
    ~AudioProvider() { }

public:
    virtual AVSampleFormat sampleFormat() const=0;  ///< Return the sample format of audio frames supplied by this provider
    virtual uint32_t sampleRate() const=0;          ///< Return the sample rate (samples/sec) of audio frames supplied by this provider
    virtual uint8_t channels() const=0;             ///< Return the number of audio channels in audio frames supplied by this provider
    virtual uint16_t samples() const=0;             ///< Return the number of samples in audio frames supplied by this provider. May not be available or accurate.
};

///
/// Specialised abstract base class for providers of both VideoFrame and AudioFrame objects.
///
class AudioVideoProvider : public VideoProvider, public AudioProvider
{
protected:
    AudioVideoProvider( const std::string &tag, const std::string &id );
    ~AudioVideoProvider() { }
};

///
/// General abstract base class for providers of all frame objects
///
class GeneralProvider : virtual public FeedProvider
{
protected:
    GeneralProvider( const std::string &tag, const std::string &id );
    GeneralProvider() { }
    ~GeneralProvider() { }

public:
};

#endif // OZ_FEED_PROVIDER_H

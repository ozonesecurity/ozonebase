/** @addtogroup Processors */
/*@{*/

#ifndef OZ_INPUT_FALLBACK_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

///
/// Processor that switches to a second feed atfer a configured interval if the primary feed fails.
///
class InputFallback : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(InputFallback);

private:
    typedef std::vector<FeedProvider *>   ProviderList;

    typedef struct {
        int         priority;
        int         frameCount;
        int64_t     lastFrameTime;
    } ProviderStats;
    typedef std::map<const FeedProvider *, ProviderStats *> ProviderStatsMap;

private:
    ProviderList        mProviderList;
    ProviderStatsMap    mProviderStats;
    const VideoProvider *mCurrentProvider;

    uint16_t            mProviderTimeout;

public:
    InputFallback( const std::string &name, double timeout=3.0 );
    ~InputFallback();

    PixelFormat pixelFormat() const { return( mCurrentProvider->pixelFormat() ); }
    uint16_t width() const { return( mCurrentProvider->width() ); }
    uint16_t height() const { return( mCurrentProvider->height() ); }
    FrameRate frameRate() const { return( mCurrentProvider->frameRate() ); }

    bool registerProvider( FeedProvider &provider, const FeedLink &link=gQueuedFeedLink );
    bool deregisterProvider( FeedProvider &provider, bool reciprocate=true );
             
protected:
    int run();
};

#endif // OZ_INPUT_FALLBACK_H

/*@}*/

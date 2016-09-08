/** @addtogroup Processors */
/*@{*/


#ifndef OZ_RATE_LIMITER_H
#define OZ_RATE_LIMITER_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

#include "../libgen/libgenThread.h"

///
/// Processor used to slow down a video stream to the specified frame rate.
///
class RateLimiter : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(RateLimiter);

private:
    bool        mSkip;          ///< Default is to skip frames, alternative is to throttle (queued only)
    FrameRate   mFrameRate;

public:
/**
* @brief 
*
* @param name name of instance
* @param frameRate rate in seconds to limit it to (can use fractions as well)
* @param skip if true, will keep itself updated with latest frame from provider, else will not skip frames as it rate limits

\code
 nvrcam.rate = new RateLimiter( "rate-"+name,0.1,true );
 nvrcam.rate->registerProvider(*(nvrcam.cam) );
\endcode
*/
    RateLimiter( const std::string &name, FrameRate frameRate, bool skip=true );
    RateLimiter( FrameRate frameRate, VideoProvider &provider );
    RateLimiter( FrameRate frameRate, bool skip, VideoProvider &provider, const FeedLink &link=gQueuedVideoLink );
    ~RateLimiter();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    PixelFormat pixelFormat() const { return( videoProvider()->pixelFormat() ); }

    FrameRate frameRate() const { return( mFrameRate ); }

protected:
    int run();
};

#endif // OZ_RATE_LIMITER_H


/*@}*/

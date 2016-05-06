#ifndef ZM_RATE_LIMITER_H
#define ZM_RATE_LIMITER_H

#include "../zmFeedBase.h"
#include "../zmFeedProvider.h"
#include "../zmFeedConsumer.h"

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
    RateLimiter( const std::string &name, FrameRate frameRate, bool skip=true );
    RateLimiter( FrameRate frameRate, bool skip, VideoProvider &provider, const FeedLink &link=gQueuedFeedLink );
    ~RateLimiter();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    PixelFormat pixelFormat() const { return( videoProvider()->pixelFormat() ); }

    FrameRate frameRate() const { return( mFrameRate ); }

protected:
    int run();
};

#endif // ZM_RATE_LIMITER_H

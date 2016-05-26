#ifndef ZM_SIGNAL_CHECKER_H
#define ZM_SIGNAL_CHECKER_H

#include "../base/zmFeedBase.h"
#include "../base/zmFeedProvider.h"
#include "../base/zmFeedConsumer.h"

#include "../libimg/libimgImage.h"

///
/// Process that checks if the received video stream is valid or whether it is 
/// a uniform colour fill normally indicating that the video signal for analogue
/// cameras has been lost. All frames are passed on regardless by default but consumers
/// can use the signalValid or signalInvalid comparator functions to filter out 
/// good or bad frames.
///
class SignalChecker : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(SignalChecker);

private:
    bool    mSignal;

public:
    SignalChecker( const std::string &name );
    ~SignalChecker();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    PixelFormat pixelFormat() const { return( videoProvider()->pixelFormat() ); }

    bool hasSignal() const { return( mSignal ); }

protected:
    int run();

protected:
    bool checkSignal( const Image &image );

public:
    static bool signalValid( FramePtr, const FeedConsumer * );
    static bool signalInvalid( FramePtr, const FeedConsumer * );
};

#endif // ZM_SIGNAL_CHECKER_H

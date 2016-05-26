#ifndef ZM_DELAY_BUFFER_H
#define ZM_DELAY_BUFFER_H

#include "../base/zmFeedBase.h"
#include "../base/zmFeedProvider.h"
#include "../base/zmFeedConsumer.h"

///
/// Processor that delays frames for a specified period
///
class DelayBufferFilter : public GeneralConsumer, public GeneralProvider, public Thread
{
CLASSID(DelayBufferFilter);

protected:
	double 	mDelay;

public:
    DelayBufferFilter( const std::string &name, double delay );
    ~DelayBufferFilter();

protected:
    int run();
};

#endif // ZM_DELAY_BUFFER_H

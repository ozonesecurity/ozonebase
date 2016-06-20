/** @addtogroup Processors */
/*@{*/


#ifndef OZ_DELAY_BUFFER_H
#define OZ_DELAY_BUFFER_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

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

#endif // OZ_DELAY_BUFFER_H


/*@}*/

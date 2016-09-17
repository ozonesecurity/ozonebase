
#include "../../base/ozApp.h"
#include "../../providers/ozAVInput.h"
#include "../../processors/ozMotionDetector.h"
#include "../../consumers/ozEventRecorder.h"
#include "../../consumers/ozMovieFileOutput.h"
#include "../../base/ozNotifyFrame.h"
#include "../../libgen/libgenDebug.h"

class NotifyOutput : public DataConsumer, public Thread
{
CLASSID(NotifyOutput);

protected:
    int run();
    bool processFrame( FramePtr );

public:
    NotifyOutput( const std::string &name ) :
        DataConsumer( cClass(), name, 8 ),
        Thread( identity() )
    {
    }
};


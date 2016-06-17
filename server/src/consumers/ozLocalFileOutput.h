/** @addtogroup Consumers */
/*@{*/

#ifndef OZ_LOCAL_FILE_OUTPUT_H
#define OZ_LOCAL_FILE_OUTPUT_H

#include "../base/ozFeedConsumer.h"
#include "../libgen/libgenThread.h"

///
/// Consumer that just just writes received video frames to files on the local filesystem
///
class LocalFileOutput : public VideoConsumer, public Thread
{
CLASSID(LocalFileOutput);

protected:
    std::string     mLocation;

protected:
    int run();
    bool writeFrame( const FeedFrame * );

public:
    LocalFileOutput( const std::string &name, const std::string &location ) :
        VideoConsumer( cClass(), name, 5 ),
        Thread( identity() ),
        mLocation( location )
    {
    }
    ~LocalFileOutput()
    {
    }
};

#endif // OZ_LOCAL_FILE_OUTPUT_H

/*@}*/

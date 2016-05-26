#ifndef ZM_LOCAL_FILE_OUTPUT_H
#define ZM_LOCAL_FILE_OUTPUT_H

#include "../base/zmFeedConsumer.h"
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

#endif // ZM_LOCAL_FILE_OUTPUT_H

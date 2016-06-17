/** @addtogroup Consumers */
/*@{*/
#ifndef OZ_LOCAL_FILE_DUMP_H
#define OZ_LOCAL_FILE_DUMP_H

#include "../base/ozFeedConsumer.h"
#include "../libgen/libgenThread.h"

///
/// Consumer that just just writes received frames to a file on the local filesystem
///
class LocalFileDump : public FeedConsumer, public Thread
{
CLASSID(LocalFileDump);

protected:
    std::string     mLocation;

protected:
    int run();
    bool writeFrame( const FeedFrame * );

public:
    LocalFileDump( const std::string &name, const std::string &location );
};

#endif // OZ_LOCAL_FILE_DUMP_H
/*@}*/

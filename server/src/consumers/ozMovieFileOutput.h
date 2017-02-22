/** @addtogroup Consumers */
/*@{*/

#ifndef OZ_MOVIE_FILE_OUTPUT_H
#define OZ_MOVIE_FILE_OUTPUT_H

#include "../base/ozFeedConsumer.h"
#include "../base/ozFeedProvider.h"
#include "../libgen/libgenThread.h"
#include "../base/ozFfmpeg.h"
#include "../base/ozOptions.h"

///
/// Consumer that just just writes received video frames to files on the local filesystem.
/// The file will be written to the given location and be called <instance name>-<timestamp>.<format>
///
class MovieFileOutput : public AudioVideoConsumer, public DataProvider, public Thread
{
CLASSID(MovieFileOutput);

protected:
    //std::string     mLocation;      ///< Path to directory in which to write movie file
    //std::string     mFormat;        ///< Short tag for movie format, e.g. mp4, mpeg
    //uint32_t        mMaxLength;     ///< Maximum length of each movie file, files will be timestamped in name
    Options         mOptions;       ///< Audio/video options

protected:
    int run();

public:
    MovieFileOutput( const std::string &name, const Options &options=gNullOptions ) :
        AudioVideoConsumer( cClass(), name ),
        Thread( identity() ),
        mOptions( options )
    {
    }
    ~MovieFileOutput()
    {
    }
};

#endif // OZ_MOVIE_FILE_OUTPUT_H
/*@}*/

/** @addtogroup Consumers */
/*@{*/

#ifndef NVR_MOVIE_FILE_OUTPUT_H
#define NVR_MOVIE_FILE_OUTPUT_H

#include <base/ozFeedConsumer.h>
#include <libgen/libgenThread.h>
#include <base/ozFfmpeg.h>
#include <consumers/ozMovieFileOutput.h>
#include <functional>

///
/// Consumer that just just writes received video frames to files on the local filesystem.
/// The file will be written to the given location and be called <instance name>-<timestamp>.<format>
///
class MovieFileOutputDetector : public VideoConsumer, public Thread
{
CLASSID(MovieFileOutputDetector);

protected:
	std::function< void(std::string) > mFunction;
    std::string     mLocation;      ///< Path to directory in which to write movie file
    std::string     mFormat;        ///< Short tag for movie format, e.g. mp4, mpeg
    uint32_t        mMaxLength;     ///< Maximum length of each movie file, files will be timestamped in name
    VideoParms      mVideoParms;    ///< Video parameters
    AudioParms      mAudioParms;    ///< Audio parameters

protected:
    int run();

public:
    MovieFileOutputDetector( const std::string &name, const std::string &location, const std::string &format, uint32_t maxLength, const VideoParms &videoParms, const AudioParms &audioParms ) :
        VideoConsumer( cClass(), name ),
        Thread( identity() ),
        mLocation( location ),
        mFormat( format ),
        mMaxLength( maxLength ),
        mVideoParms( videoParms ),
        mAudioParms( audioParms )
    {
    }
    MovieFileOutputDetector( const std::string &name, const std::string &location, const std::string &format, uint32_t maxLength, const VideoParms &videoParms, const AudioParms &audioParms, std::function<void(std::string)> mFunc) :
        VideoConsumer( cClass(), name ),
        Thread( identity() ),
        mLocation( location ),
        mFormat( format ),
        mMaxLength( maxLength ),
        mVideoParms( videoParms ),
        mAudioParms( audioParms ),
		mFunction(mFunc)
    {
    }
~MovieFileOutputDetector()
    {
    }
};

#endif // NVR_MOVIE_FILE_OUTPUT_H
/*@}*/

/** @addtogroup Processors */
/*@{*/

#ifndef OZ_VIDEO_FILTER_H
#define OZ_VIDEO_FILTER_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

class Rational;

///
/// Processor that applies an ffmpeg filter to video frames
///
class VideoFilter : public VideoConsumer, public VideoProvider, public Thread
{
CLASSID(VideoFilter);

private:
    std::string         mFilter;

    AVFilterContext *mBufferSrcContext;
    AVFilterContext *mBufferSinkContext;
    AVFilterGraph *mFilterGraph;

public:
    VideoFilter( const std::string &name, const std::string &filter );
    VideoFilter( const std::string &filter, VideoProvider &provider, const FeedLink &link=gQueuedVideoLink );
    ~VideoFilter();

    uint16_t width() const;
    uint16_t height() const;
    AVPixelFormat pixelFormat() const;
    FrameRate frameRate() const;

    const std::string &filter() const { return( mFilter ); }

protected:
    int run();
};

#endif // OZ_VIDEO_FILTER_H

/*@}*/

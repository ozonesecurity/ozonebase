/** @addtogroup Processors */
/*@{*/

#ifndef OZ_FILTER_H
#define OZ_FILTER_H

#include "../base/ozFeedBase.h"
#include "../base/ozFeedProvider.h"
#include "../base/ozFeedConsumer.h"

class Rational;

///
/// Object that applies an ffmpeg filter to individual images
///
class ImageFilter
{
private:
    std::string     mFilter;

    uint16_t        mInputWidth;
    uint16_t        mInputHeight;
    PixelFormat     mInputPixelFormat;

    uint16_t        mOutputWidth;
    uint16_t        mOutputHeight;
    PixelFormat     mOutputPixelFormat;

    std::string     mSignature;

    AVFilterContext *mBufferSrcContext;
    AVFilterContext *mBufferSinkContext;

    AVFilter        *mFilterBufferSource;
    AVFilter        *mFilterBufferSink;

    AVFilterInOut   *mFilterOutputs;
    AVFilterInOut   *mFilterInputs;

    AVFilterGraph   *mFilterGraph;

    AVFrame         *mInputFrame;
    AVFrame         *mFilterFrame;

    ByteBuffer      mOutputBuffer;

public:
    ImageFilter( const std::string &filter, uint16_t inputWidth, uint16_t inputHeight, PixelFormat inputPixelFormat );
    ImageFilter( const std::string &filter, const Image &inputImage );
    ~ImageFilter();

    const std::string &filter() const { return( mFilter ); }
    const std::string &signature() const { return( mSignature ); }

    Image *execute( const Image &inputImage );
};

#endif // OZ_FILTER_H

/*@}*/

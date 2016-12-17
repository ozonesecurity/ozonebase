/** @addtogroup Processors */
/*@{*/

#ifndef OZ_RECOGNIZER_H
#define OZ_RECOGNIZER_H

#include "../base/ozDetector.h"
#include "../base/ozOptions.h"

///
/// Processor that detects faces on a video frame feed 
/// This processor uses dlib and also requires presence
/// of the dlib shape detector dat file
///  You can get the shape_predictor_68_face_landmarks.dat file from:
///   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2

class Recognizer : public virtual Detector
{
CLASSID(Recognizer);

public:
    typedef enum { OZ_RECOGNIZER_OPENALPR=0x0001, OZ_RECONIZER_OPENCV_OCR=0x0002 } RecognizerFlags;

private:
    uint16_t    mFlags;
    Options     mOptions;

public:
    Recognizer( const std::string &name, uint16_t flags, const Options &options=gNullOptions );
    Recognizer( VideoProvider &provider, uint16_t flags, const Options &options=gNullOptions, const FeedLink &link=gQueuedVideoLink );
    ~Recognizer();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    AVPixelFormat pixelFormat() const { return( Image::getFfPixFormat( Image::FMT_RGB ) ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

protected:
    int run();
};

#endif // OZ_RECOGNIZER_H

/*@}*/

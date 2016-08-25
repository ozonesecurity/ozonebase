/** @addtogroup Processors */
/*@{*/

#ifndef OZ_FACE_DETECTOR_H
#define OZ_FACE_DETECTOR_H

#include "../base/ozDetector.h"


#include <set>
#include <map>
#include <list>

class MotionData;

///
/// Processor that detects motion on a video frame feed and reports on the nature of the motion via
/// MotionFrame objects.
///
class FaceDetector : public virtual Detector
{
CLASSID(FaceDetector);

public:
    FaceDetector( const std::string &name );
    ~FaceDetector();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    AVPixelFormat pixelFormat() const { return( Image::getFfPixFormat( Image::FMT_RGB ) ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

protected:
    int run();
};

#endif // OZ_FACE_DETECTOR_H

/*@}*/

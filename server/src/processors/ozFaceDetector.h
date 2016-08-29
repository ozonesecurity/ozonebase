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
/// Processor that detects faces on a video frame feed 
/// This processor uses dlib and also requires presence
/// of the dlib shape detector dat file in the same directory
/// as the invoking application as of now
///  You can get the shape_predictor_68_face_landmarks.dat file from:
///   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2

class FaceDetector : public virtual Detector
{
CLASSID(FaceDetector);

public:
/**
* @brief 
*
* @param name Name of instance

\code
nvrcam.face = new FaceDetector( "face-cam0" );
\endcode
*/
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

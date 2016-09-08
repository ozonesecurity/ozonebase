/** @addtogroup Processors */
/*@{*/

#ifndef OZ_SHAPE_DETECTOR_H
#define OZ_SHAPE_DETECTOR_H

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

class ShapeDetector : public virtual Detector
{
CLASSID(ShapeDetector);

public:
    typedef enum { OZ_SHAPE_MARKUP_NONE=0, OZ_SHAPE_MARKUP_OUTLINE=1, OZ_SHAPE_MARKUP_ALL=0xff } ShapeMarkup;

private:
    std::string mObjectData;
    ShapeMarkup mShapeMarkup;

/**
* @brief 
*
* @param name Name of instance

\code
nvrcam.shape = new ShapeDetector( "shape-cam0" );
\endcode
*/
public:
    ShapeDetector( const std::string &name, const std::string &objectData, ShapeMarkup shapeMarkup=OZ_SHAPE_MARKUP_ALL );
    ShapeDetector( const std::string &objectData, ShapeMarkup faceMarkup, VideoProvider &provider, const FeedLink &link=gQueuedFeedLink );
    ~ShapeDetector();

    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    AVPixelFormat pixelFormat() const { return( Image::getFfPixFormat( Image::FMT_RGB ) ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

protected:
    int run();
};

#endif // OZ_SHAPE_DETECTOR_H

/*@}*/

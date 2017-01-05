/** @addtogroup Processors */
/*@{*/

#ifndef OZ_FACE_DETECTOR_H
#define OZ_FACE_DETECTOR_H

#include "../base/ozDetector.h"
#include "../base/ozOptions.h"

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>
#include <dlib/image_io.h>

#include <set>
#include <map>
#include <list>

class MotionData;

///
/// Processor that detects faces on a video frame feed 
/// This processor uses dlib and also requires presence
/// of the dlib shape detector dat file
///  You can get the shape_predictor_68_face_landmarks.dat file from:
///   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2

class FaceDetector : public virtual Detector
{
CLASSID(FaceDetector);

public:
    typedef enum { OZ_FACE_MARKUP_NONE=0, OZ_FACE_MARKUP_OUTLINE=1, OZ_FACE_MARKUP_DETAIL=2, OZ_FACE_MARKUP_ALL=3 } FaceMarkup;

private:
	class DlibDetector {
	protected:
        FaceDetector    *mFaceDetector;
		FaceMarkup	    mMarkup;
	protected:
		DlibDetector( FaceDetector *faceDetector, FaceMarkup markup ) :
            mFaceDetector( faceDetector ),
            mMarkup( markup )
        {
        }
	public:
		virtual ~DlibDetector() {};
        virtual int detect( const ByteBuffer &inputBuffer, ByteBuffer &outputBuffer ) = 0;
	};

	class DlibHogDetector : public DlibDetector {
	private:
		dlib::frontal_face_detector mDetector;
		dlib::shape_predictor mShapePredictor;
	public:
		DlibHogDetector( FaceDetector *faceDetector, const std::string &dataFile, FaceMarkup markup );
        int detect( const ByteBuffer &inputBuffer, ByteBuffer &outputBuffer );
	};

private:
	Options			mOptions;
	DlibDetector	*mDetector;

private:
	void construct();

public:
    FaceDetector( const std::string &name, const Options &options=gNullOptions );
    FaceDetector( VideoProvider &provider, const Options &options=gNullOptions, const FeedLink &link=gQueuedVideoLink );
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

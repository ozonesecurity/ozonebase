#include "../base/oz.h"
#include "ozFaceDetector.h"

#include "../base/ozAlarmFrame.h"

#include <iostream>
#include <vector>



FaceDetector::DlibHogDetector::DlibHogDetector( FaceDetector *faceDetector, const std::string &dataFile, FaceMarkup markup ) :
    DlibDetector( faceDetector, markup )
{
    // We need a face detector.  We will use this to get bounding boxes for
    // each face in an image.
    mDetector = dlib::get_frontal_face_detector();

    // And we also need a shape_predictor.  This is the tool that will predict face
    // landmark positions given an image and face bounding box.  Here we are just
    // loading the model from the shape_predictor_68_face_landmarks.dat file you gave
    // as a command line argument.
    
    //std::cout << "---------- HOLY MOOLA" << std::endl;
    dlib::deserialize( dataFile.c_str() ) >> mShapePredictor;
    //dlib::deserialize( "shape_predictor_68_face_landmarks.dat" ) >> mShapePredictor;
    //dlib::deserialize( dataFile ) >> mShapePredictor;
}

int FaceDetector::DlibHogDetector::detect( const ByteBuffer &inputBuffer, ByteBuffer &outputBuffer )
{
    dlib::array2d<dlib::rgb_pixel> img( mFaceDetector->height(), mFaceDetector->width() );
    memcpy( image_data( img ), inputBuffer.data(), inputBuffer.size() );
    //assign_image( img, frame->buffer().data() );

    //Info( "B4: %d x %d", num_rows(img), num_columns(img) );
    //dlib::pyramid_up(img);
    //Info( "AFT: %d x %d", num_rows(img), num_columns(img) );
    std::vector<dlib::rectangle> dets = mDetector(img);

    if ( dets.size() > 0 && mMarkup != OZ_FACE_MARKUP_NONE )
    {
        // Now we will go ask the shape_predictor to tell us the pose of
        // each face we detected.
        std::vector<dlib::full_object_detection> shapes;
        const dlib::rgb_pixel color = dlib::rgb_pixel(0,255,0);
        typedef std::pair<dlib::point,dlib::point> line_t;
        std::vector<line_t> lines;
        for ( unsigned int i = 0; i < dets.size(); i++ )
        {
            if ( mMarkup & OZ_FACE_MARKUP_OUTLINE )
                draw_rectangle( img, dets[i], dlib::rgb_pixel( 255, 0, 0 ), 1 );

            if ( mMarkup & OZ_FACE_MARKUP_DETAIL )
            {
                dlib::full_object_detection shape = mShapePredictor(img, dets[i]);
                Debug( 2, "Face %u - %ju parts", i, shape.num_parts() );

                // Around Chin. Ear to Ear
                for (unsigned long p = 1; p <= 16; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));

                // Line on top of nose
                for (unsigned long p = 28; p <= 30; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));

                // left eyebrow
                for (unsigned long p = 18; p <= 21; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));
                // Right eyebrow
                for (unsigned long p = 23; p <= 26; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));
                // Bottom part of the nose
                for (unsigned long p = 31; p <= 35; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));
                // Line from the nose to the bottom part above
                lines.push_back(line_t(shape.part(30), shape.part(35)));

                // Left eye
                for (unsigned long p = 37; p <= 41; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));
                lines.push_back(line_t(shape.part(36), shape.part(41)));

                // Right eye
                for (unsigned long p = 43; p <= 47; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));
                lines.push_back(line_t(shape.part(42), shape.part(47)));

                // Lips outer part
                for (unsigned long p = 49; p <= 59; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));
                lines.push_back(line_t(shape.part(48), shape.part(59)));

                // Lips inside part
                for (unsigned long p = 61; p <= 67; ++p)
                    lines.push_back(line_t(shape.part(p), shape.part(p-1)));
                lines.push_back(line_t(shape.part(60), shape.part(67)));

                shapes.push_back(shape);
            }
        }

        if ( mMarkup & OZ_FACE_MARKUP_DETAIL )
        {
            for ( std::vector<line_t>::const_iterator lineIter = lines.begin(); lineIter != lines.end(); lineIter++ )
            {
                const line_t &line = *lineIter;
                dlib::draw_line( img, line.first, line.second, color );
            }
        }

        //Info( "%d x %d = %d", num_rows(img), num_columns(img), 3*num_rows(img)*num_columns(img) );
        //dlib::save_png( img, "/transfer/image.png" );
    }
    // Don't really need to do this if nothing detected as will be the same as input buffer
    if ( dets.size() > 0 )
        outputBuffer.assign( (uint8_t *)image_data(img), 3*num_rows(img)*num_columns(img) );
    return ( dets.size() );
}

FaceDetector::DlibCnnDetector::DlibCnnDetector( FaceDetector *faceDetector, const std::string &dataFile, FaceMarkup markup ) :
    DlibDetector( faceDetector, markup )
{
     //Debug (1, "SERIALIZING %s",dataFile.c_str());
     dlib::deserialize( dataFile.c_str() ) >> mDetector;
}

int FaceDetector::DlibCnnDetector::detect( const ByteBuffer &inputBuffer, ByteBuffer &outputBuffer )
{
    dlib::matrix<dlib::rgb_pixel> img( mFaceDetector->height(), mFaceDetector->width() );
    memcpy( image_data( img ), inputBuffer.data(), inputBuffer.size() );

    //while(img.size() < 1800*1800)
        //dlib::pyramid_up(img);

    auto dets = mDetector(img);
    //std::vector<dlib::rectangle> dets = mDetector(img);

    if ( dets.size() > 0 && mMarkup != OZ_FACE_MARKUP_NONE )
    {
        // Now we will go ask the shape_predictor to tell us the pose of
        // each face we detected.
        std::vector<dlib::full_object_detection> shapes;
        typedef std::pair<dlib::point,dlib::point> line_t;
        std::vector<line_t> lines;
        for ( unsigned int i = 0; i < dets.size(); i++ )
        {
            if ( mMarkup & OZ_FACE_MARKUP_OUTLINE )
                draw_rectangle( img, dets[i], dlib::rgb_pixel( 255, 0, 0 ), 1 );
        }

        //Info( "%d x %d = %d", num_rows(img), num_columns(img), 3*num_rows(img)*num_columns(img) );
        //dlib::save_png( img, "/transfer/image.png" );
    }
    // Don't really need to do this if nothing detected as will be the same as input buffer
    if ( dets.size() > 0 )
        outputBuffer.assign( (uint8_t *)image_data(img), 3*num_rows(img)*num_columns(img) );
    return ( dets.size() );
}

void FaceDetector::construct()
{
    std::string method = mOptions.get( "method", "hog" );
    std::string dataFile = mOptions.get( "dataFile", "" );
    if ( !dataFile.length() )
        throw "No data file specified";
    FaceMarkup markup = mOptions.get( "markup", OZ_FACE_MARKUP_OUTLINE );
    if ( method == "hog" )
        mDetector = new DlibHogDetector( this, dataFile, markup );
    else if ( method == "cnn" )
        mDetector = new DlibCnnDetector( this, dataFile, markup );
    else
        throw "Invalid face detection method '"+method+"' specifed";
}

/**
* @brief
*
* @param name
*/
FaceDetector::FaceDetector( const std::string &name, const Options &options ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mOptions( options ),
    mDetector( NULL )
{
    construct();
}

/**
* @brief
*
* @param markup
* @param provider
* @param link
*/
FaceDetector::FaceDetector( VideoProvider &provider, const Options &options, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mOptions( options ),
    mDetector( NULL )
{
    construct();
}

/**
* @brief
*/
FaceDetector::~FaceDetector()
{
    delete mDetector;
}

/**
* @brief
*
* @return
*/
int FaceDetector::run()
{
    // Wait for encoder to be ready
    if ( waitForProviders() )
    {
        AVPixelFormat pixelFormat = videoProvider()->pixelFormat();
        int16_t width = videoProvider()->width();
        int16_t height = videoProvider()->height();
        Debug( 2, "pf:%d, %dx%d", pixelFormat, width, height );
        ByteBuffer detectBuffer;

        setReady();
        while ( !mStop )
        {
            if ( mStop )
               break;
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                Debug( 3, "Got %zd frames on queue", mFrameQueue.size() );
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    // Only operate on video frames
                    const VideoFrame *frame = dynamic_cast<const VideoFrame *>(iter->get());
                    if ( frame )
                    {
                        Image image( pixelFormat, width, height, frame->buffer().data() );
                        Image convImage( Image::FMT_RGB, image );
                        int detected = mDetector->detect( convImage.buffer(), detectBuffer );
                        Debug( detected > 0 ?1:2, "%jd @ %ju: Got %d faces", frame->id(), frame->timestamp(), detected );
                        AlarmFrame *alarmFrame = NULL;
                        if ( detected )
                            alarmFrame = new AlarmFrame( this, *iter, frame->id(), frame->timestamp(), detectBuffer, detected );
                        else
                            alarmFrame = new AlarmFrame( this, *iter, frame->id(), frame->timestamp(), convImage.buffer(), detected );
                        distributeFrame( FramePtr( alarmFrame ) );
                        mFrameCount++;
                    }
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            // Quite short so we can always keep up with the required packet rate for 25/30 fps
            usleep( INTERFRAME_TIMEOUT );
        }
    }
    FeedProvider::cleanup();
    FeedConsumer::cleanup();
    return( !ended() );
}

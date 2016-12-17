#include "../base/oz.h"
#include "ozShapeDetector.h"

#include "../base/ozAlarmFrame.h"
#include <dlib/svm_threaded.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>
#include <iostream>
#include <vector>

/**
* @brief
*
* @param name
*/
ShapeDetector::ShapeDetector( const std::string &name, const std::string &objectData, ShapeMarkup shapeMarkup ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mObjectData( objectData ),
    mShapeMarkup( shapeMarkup )
{
}

/**
* @brief
*
* @param markup
* @param provider
* @param link
*/
ShapeDetector::ShapeDetector( const std::string &objectData, ShapeMarkup shapeMarkup, VideoProvider &provider, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mObjectData( objectData ),
    mShapeMarkup( shapeMarkup )
{
}

/**
* @brief
*/
ShapeDetector::~ShapeDetector()
{
}

/**
* @brief
*
* @return
*/
int ShapeDetector::run()
{
    config.dir_events = "/transfer";
    config.record_diag_images = true;

    // Wait for encoder to be ready
    if ( waitForProviders() )
    {
        AVPixelFormat pixelFormat = videoProvider()->pixelFormat();
        int16_t width = videoProvider()->width();
        int16_t height = videoProvider()->height();
        Debug( 1,"pf:%d, %dx%d", pixelFormat, width, height );

        typedef dlib::scan_fhog_pyramid<dlib::pyramid_down<6> > image_scanner_type;

        dlib::object_detector<image_scanner_type> detector;
        dlib::deserialize( mObjectData.c_str() ) >> detector;

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
                        dlib::array2d<dlib::rgb_pixel> img( height, width );
                        memcpy( image_data( img ), convImage.buffer().data(), convImage.buffer().size() );
                        //assign_image( img, frame->buffer().data() );

                        //Info( "B4: %d x %d", num_rows(img), num_columns(img) );
                        //dlib::pyramid_up(img);
                        //Info( "AFT: %d x %d", num_rows(img), num_columns(img) );
                        std::vector<dlib::rectangle> dets = detector(img);

                       	Debug( (dets.size() > 0) ? 1:2,"%jd @ %ju: Got %jd shapes", frame->id(), frame->timestamp(), dets.size() );
                        if ( dets.size() > 0 && mShapeMarkup != OZ_SHAPE_MARKUP_NONE )
                        {
                            // Now we will go ask the shape_predictor to tell us the pose of
                            // each shape we detected.
                            std::vector<dlib::full_object_detection> shapes;
                            typedef std::pair<dlib::point,dlib::point> line_t;
                            std::vector<line_t> lines;
                            for ( unsigned int i = 0; i < dets.size(); i++ )
                            {
                                if ( mShapeMarkup & OZ_SHAPE_MARKUP_OUTLINE )
                                    draw_rectangle( img, dets[i], dlib::rgb_pixel( 255, 0, 0 ), 1 );
                            }

                            //dlib::save_png( img, "/transfer/image.png" );
                            //Info( "%d x %d = %d", num_rows(img), num_columns(img), 3*num_rows(img)*num_columns(img) );
                        }
						else
						{
                        	Debug( 2,"%jd @ %ju: Got %jd shapes", frame->id(), frame->timestamp(), dets.size() );
						}
                        // Move inside preceding 'if' to only output 'shape' frames
                        AlarmFrame *alarmFrame = new AlarmFrame( this, *iter, frame->id(), frame->timestamp(), (uint8_t *)image_data(img), 3*num_rows(img)*num_columns(img), dets.size()>0 );

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

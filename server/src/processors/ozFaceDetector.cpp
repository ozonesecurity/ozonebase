#include "../base/oz.h"
#include "ozFaceDetector.h"

#include "../base/ozAlarmFrame.h"
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_io.h>
#include <vector>

/**
* @brief 
*
* @param name
*/
FaceDetector::FaceDetector( const std::string &name ) :
    VideoProvider( cClass(), name ),
    Thread( identity() )
{
}

/**
* @brief 
*/
FaceDetector::~FaceDetector()
{
}

/**
* @brief 
*
* @return 
*/
int FaceDetector::run()
{
    config.dir_events = "/transfer";
    config.record_diag_images = true;

    // Wait for encoder to be ready
    if ( waitForProviders() )
    {
        AVPixelFormat pixelFormat = videoProvider()->pixelFormat();
        int16_t width = videoProvider()->width();
        int16_t height = videoProvider()->height();
        Info( "pf:%d, %dx%d", pixelFormat, width, height );

        dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();

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

                        std::vector<dlib::rectangle> dets = detector(img);
                        Info( "%jd @ %ju: Got %jd faces", frame->id(), frame->timestamp(), dets.size() );

                        if ( dets.size() > 0 )
                        {
                            for ( unsigned int i = 0; i < dets.size(); i++ )
                            {
                                draw_rectangle( img, dets[i], dlib::rgb_pixel( 255, 0, 0 ), 1 );
                            }
                            //dlib::save_png( img, "/transfer/image.png" );
                            //Info( "%d x %d = %d", num_rows(img), num_columns(img), 3*num_rows(img)*num_columns(img) );
                            VideoFrame *imageFrame = new VideoFrame( this, *iter, mFrameCount, time64(), (uint8_t *)image_data(img), 3*num_rows(img)*num_columns(img) );

                            distributeFrame( FramePtr( imageFrame ) );
                            mFrameCount++;
                        }
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

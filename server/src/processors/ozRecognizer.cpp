#include "../base/oz.h"
#include "ozRecognizer.h"

#include "../base/ozFilter.h"
#include "../base/ozAlarmFrame.h"
#include <alpr.h>
#include <iostream>
#include <vector>

/**
* @brief
*
* @param name
*/
Recognizer::Recognizer( const std::string &name, uint16_t flags, const Options &options ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mFlags( flags ),
    mOptions( options )
{
}

/**
* @brief
*
* @param markup
* @param provider
* @param link
*/
Recognizer::Recognizer( VideoProvider &provider, uint16_t flags, const Options &options, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mFlags( flags ),
    mOptions( options )
{
}

/**
* @brief
*/
Recognizer::~Recognizer()
{
}

/**
* @brief
*
* @return
*/
int Recognizer::run()
{
    alpr::Alpr *openalpr = NULL;
    if ( mFlags & OZ_RECOGNIZER_OPENALPR )
    {
        std::string alprConfig = mOptions.get( "alpr.config", "openalpr.conf" );
        std::string alprCountry = mOptions.get( "alpr.country", "gb" );
        std::string alprRegion = mOptions.get( "alpr.region", "" );
        int alprLimit = mOptions.get( "alpr.limit", 10 );

        openalpr = new alpr::Alpr( alprCountry.c_str(), alprConfig.c_str() );

        // Optionally specify the top N possible plates to return (with confidences).  Default is 10
        openalpr->setTopN( alprLimit );

        if ( alprRegion.length() )
        {
            // Optionally, provide the library with a region for pattern matching.  This improves accuracy by
            // comparing the plate text with the regional pattern.
            openalpr->setDefaultRegion( alprRegion.c_str() );
        }

        // Make sure the library loaded before continuing.
        // For example, it could fail if the config/runtime_data is not found
        if ( openalpr->isLoaded() == false )
        {
            Fatal( "Can't load openALPR library" );
        }
    }

    // Wait for encoder to be ready
    if ( waitForProviders() )
    {
        AVPixelFormat pixelFormat = videoProvider()->pixelFormat();
        int16_t width = videoProvider()->width();
        int16_t height = videoProvider()->height();
        Debug( 2, "pf:%d, %dx%d", pixelFormat, width, height );

        //ImageFilter filter( "format=pix_fmts=rgb24", width, height, pixelFormat );

        //setReady();
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
                        //Image convImage( Image::FMT_RGB, image );
                        //Image *convImage = filter.execute( image );
                        //std::vector::<char> img( height, width );
                        //memcpy( image_data( img ), convImage.buffer().data(), convImage.buffer().size() );
                        //assign_image( img, frame->buffer().data() );

                        if ( openalpr )
                        {
                            unsigned char *imageData = image.buffer().data();
                            std::vector<alpr::AlprRegionOfInterest> roiList;
                            //std::vector<char> alprData( imageData, imageData+convImage.buffer().length() );

                            // Recognize an image file.  You could alternatively provide the image bytes in-memory.
                            //alpr::AlprResults results = openalpr->recognize( alprData );
                            alpr::AlprResults results = openalpr->recognize( imageData, 3, width, height, roiList );

                            Info( "Got %d candidates in %.2fms", results.plates.size(), results.total_processing_time_ms );

                            // Iterate through the results.  There may be multiple plates in an image,
                            // and each plate return sthe top N candidates.
                            for (int i = 0; i < results.plates.size(); i++)
                            {
                                alpr::AlprPlateResult plate = results.plates[i];
                                std::cout << "plate" << i << ": " << plate.topNPlates.size() << " results" << std::endl;

                                for (int k = 0; k < plate.topNPlates.size(); k++)
                                {
                                    alpr::AlprPlate candidate = plate.topNPlates[k];
                                    std::cout << "    - " << candidate.characters << "\t confidence: " << candidate.overall_confidence;
                                    std::cout << "\t pattern_match: " << candidate.matches_template << std::endl;
                                }
                            }

                            // Move inside preceding 'if' to only output 'face' frames
                            //AlarmFrame *alarmFrame = new AlarmFrame( this, *iter, frame->id(), frame->timestamp(), (uint8_t *)image_data(img), 3*num_rows(img)*num_columns(img), dets.size()>0 );
                            //distributeFrame( FramePtr( alarmFrame ) );
                        }
                        //mFrameCount++;
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

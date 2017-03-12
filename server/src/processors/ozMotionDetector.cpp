#include "../base/oz.h"
#include "ozMotionDetector.h"

#include "../base/ozAlarmFrame.h"
#include "../base/ozZone.h"

#include <sys/time.h>

/**
* @brief 
*/
void MotionDetector::construct()
{
    //
    mDebugStreams = mOptions.get( "debug_streams", true );
    // Write out images to see what is going on?
    mDebugImages = mOptions.get( "debug_images", false );
    // If so where?
    mDebugLocation = mOptions.get( "debug_location", "/tmp" );

    mRefBlend = 1 << mOptions.get( "options_refBlend", 7 ); // 128
    mVarBlend = 1 << mOptions.get( "options_varBlend", 9 ); // 512

    // Include alarmed images in the reference image?
    mBlendAlarmedImages = mOptions.get( "options_blendAlarmedImages", true );

    // Adjust for wholesale brightness changes?
    mDeltaCorrection = mOptions.get( "options_deltaCorrection", false );

    mAnalysisScale = mOptions.get( "options_analysisScale", 2 );

    mZoneDefaults.mAlarmRgb = mOptions.get( "zone_default_color", (Rgb)RGB_RED );
    mZoneDefaults.mCheckBlobs = mOptions.get( "zone_default_checkBlobs", true );
    mZoneDefaults.mDiffThres = mOptions.get( "zone_default_diffThres", 2.0 );
    mZoneDefaults.mScoreThres = mOptions.get( "zone_default_scoreThres", 1.41 );
    mZoneDefaults.mScoreBlend = mOptions.get( "zone_default_scoreBlend", 64 );
    mZoneDefaults.mMinAlarmPercent = mOptions.get( "zone_default_alarmPercent_min", 5.0 );
    mZoneDefaults.mMaxAlarmPercent = mOptions.get( "zone_default_alarmPercent_max", 0.0 );
    mZoneDefaults.mMinScore = mOptions.get( "zone_default_score_min", 50 );
    mZoneDefaults.mMaxScore = mOptions.get( "zone_default_score_max", 0 );
    mZoneDefaults.mScale = mAnalysisScale;

    mColourDeltas = false;

    mAlarmCount = 0;
    mFastStart = true;
    mReadyCount = mFastStart?min(mRefBlend,mVarBlend)/2:max(mRefBlend,mVarBlend);
    mFirstAlarmCount = 0;
    mLastAlarmCount = 0;
    mAlarmed = false;

    mNoiseLevel = 8;
    mNoiseLevelSq = (mNoiseLevel*mNoiseLevel) << 16;

    mStartTime = time( 0 );

    mCompImageSlave = new SlaveVideo( mName+"-compImage" );
    mRefImageSlave = new SlaveVideo( mName+"-refImage" );
    mDeltaImageSlave = new SlaveVideo( mName+"-deltaImage" );
    mVarImageSlave = new SlaveVideo( mName+"-varImage" );
}

/**
* @brief 
*
* @param name
*/
MotionDetector::MotionDetector( const std::string &name, const Options &options ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mOptions( options ),
    mVarImage( NULL ),
    mCompImageSlave( NULL ),
    mRefImageSlave( NULL ),
    mDeltaImageSlave( NULL ),
    mVarImageSlave( NULL )
{
    construct();
}

/**
* @brief 
*/
MotionDetector::MotionDetector( VideoProvider &provider, const Options &options, const FeedLink &link ) :
    VideoConsumer( cClass(), provider, link ),
    VideoProvider( cClass(), provider.name() ),
    Thread( identity() ),
    mOptions( options ),
    mVarImage( NULL ),
    mCompImageSlave( NULL ),
    mRefImageSlave( NULL ),
    mDeltaImageSlave( NULL ),
    mVarImageSlave( NULL )
{
    construct();
}

/**
* @brief 
*/
MotionDetector::~MotionDetector()
{
    for ( ZoneSet::iterator zoneIter = mZones.begin(); zoneIter!= mZones.end(); zoneIter++ )
        delete *zoneIter;
    mZones.clear();
    delete mVarImage;

    mAlarmed = false;
}

/**
* @brief 
*
* @param zone
*
* @return 
*/
bool MotionDetector::addZone( Zone *zone )
{
    std::pair<ZoneSet::iterator,bool> result = mZones.insert( zone );
    return( result.second );
}

/**
* @brief 
*
* @return 
*/
int MotionDetector::run()
{
    // Wait for encoder to be ready
    if ( waitForProviders() )
    {
        AVPixelFormat pixelFormat = videoProvider()->pixelFormat();
        int16_t width = videoProvider()->width();
        int16_t height = videoProvider()->height();
        Debug( 2,"pf:%d, %dx%d", pixelFormat, width, height );

        if ( !mZones.size() )
        {
            Coord coords[4] = { Coord( 0, 0 ), Coord( width-1, 0 ), Coord( width-1, height-1 ), Coord( 0, height-1 ) };
            addZone( new Zone( 0, "All", Zone::ACTIVE, Polygon( sizeof(coords)/sizeof(*coords), coords ), mZoneDefaults ) );
        }

        mCompImageSlave->prepare( videoProvider()->frameRate() );
        mRefImageSlave->prepare( videoProvider()->frameRate() );
        mDeltaImageSlave->prepare( videoProvider()->frameRate() );
        mVarImageSlave->prepare( videoProvider()->frameRate() );

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
                        //Image image( Image::FMT_YUVP, width, height, frame->buffer().data() );
                        //Image *image = new Image( Image::FMT_YUVP, width, height, packet->data() );
                        if ( mRefImage.empty() )
                        {
                            if ( mDebugImages )
                                image.writeJpeg( stringtf( "%s/image-%s-%ju.jpg", mDebugLocation.c_str(), mName.c_str(), frame->id() ), 100 );
                            mRefImage.assign( Image( Image::FMT_RGB48, image ) );
                            if ( mDebugImages )
                                mRefImage.writeJpeg( stringtf( "%s/ref-%s-%ju.jpg", mDebugLocation.c_str(), mName.c_str(), frame->id() ), 100 );
                        }

                        //struct timeval *timestamp = new struct timeval;
                        //gettimeofday( timestamp, NULL );

                        MotionData *motionData = new MotionData;
                        //Image motionImage( Image::FMT_RGB, image );
                        Image motionImage( image );
                        //motionImage.erase();
                        //motionData.reset();

                        analyse( &image, motionData, &motionImage );
                        /*if ( mAlarmed )
                        {

                           Info( "ALARM" );
                        }*/
                        // XXX - Generate a new timestamp as using the original frame may be subject to process/decoding delays
                        // Need to check if there is a better way to handle this generally
                        MotionFrame *motionFrame = new MotionFrame( this, *iter, mFrameCount, time64(), motionImage.buffer(), mAlarmed, motionData );

                        distributeFrame( FramePtr( motionFrame ) );

                        if ( mBlendAlarmedImages || !mAlarmed )
                        {
                            int refBlend = mRefBlend;
                            if ( refBlend > (mFrameCount+1) )
                            {
                                while ( refBlend > (mFrameCount+1) )
                                    refBlend >>= 1;
                                //printf( "Frame: %d, adjusting refBlend = %d\n", frameCount, refBlend );
                            }
                            mRefImage.blend( image, refBlend );
                        }
                        //mRefImage.blend( image, mRefBlend );
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
    delete mCompImageSlave;
    delete mRefImageSlave;
    delete mDeltaImageSlave;
    delete mVarImageSlave;
    return( !ended() );
}

/**
* @brief 
*
* @param compImage
* @param motionData
* @param motionImage
*
* @return 
*/
bool MotionDetector::analyse( const Image *compImage, MotionData *motionData, Image *motionImage )
{
    double score = 0;
    if ( mFrameCount > mReadyCount )
    {
        std::string cause;
        StringSetMap noteSetMap;

        ZoneSet detectedZones;
        uint64_t motionScore = detectMotion( *compImage, detectedZones );
        if ( motionScore )
        {
            Debug(1, "Score: %ld", motionScore );
            if ( !mAlarmed )
            {
                score += motionScore;
                if ( cause.length() )
                    cause += ", ";
                cause += MOTION_CAUSE;
            }
            else
            {
                score += motionScore;
            }
            StringSet zoneLabels;
            for ( ZoneSet::iterator zoneIter = detectedZones.begin(); zoneIter!= detectedZones.end(); zoneIter++ )
            {
                Zone *zone = *zoneIter;
                if ( motionData && zone->motionData() )
                    *motionData += *(zone->motionData());
                if ( motionImage )
                    motionImage->overlay( *(zone->motionImage()) );
                zoneLabels.insert( zone->label() );
            }
            noteSetMap[MOTION_CAUSE] = zoneLabels;
        }
        if ( score )
        {
            if ( !mAlarmed )
            {
                Info( "%03ju - Gone into alarm state", mFrameCount );
                mAlarmed = true;
            }
        }
        else
        {
            if ( mAlarmed )
            {
                Info( "%03ju - Left alarm state", mFrameCount );
                mAlarmed = false;
            }
        }
#if 0
        if ( mAlarmed )
        {
            if ( motionData && config.create_analysis_images )
            {
                bool gotAnalImage = false;
                Image alarmImage( *compImage );
                alarmImage.overlay( *(motionData->image()) );
                /*
                for ( ZoneSet::iterator zoneIter = mZones.begin(); zoneIter!= mZones.end(); zoneIter++ )
                {
                    Zone *zone = *zoneIter;
                    if ( zone->isAlarmed() )
                    {
                        const MotionData *motionData = zone->motionData();
                        alarmImage.overlay( *(motionData->image()) );
                        gotAnalImage = true;
                        if ( config.record_event_stats && mAlarmed )
                        {
                            //zone->recordStats( mEvent );
                        }
                    }
                }
                */
            }
            else
            {
                for ( ZoneSet::iterator zoneIter = mZones.begin(); zoneIter!= mZones.end(); zoneIter++ )
                {
                    Zone *zone = *zoneIter;
                    if ( zone->isAlarmed() )
                    {
                        if ( config.record_event_stats && mAlarmed )
                        {
                            //mZones[i]->recordStats( mEvent );
                        }
                    }
                }
            }
        }
#endif
    }
    return( mAlarmed );
}

/**
* @brief 
*
* @param compImage
* @param motionZones
*
* @return 
*/
uint32_t MotionDetector::detectMotion( const Image &compImage, ZoneSet &motionZones )
{
    bool alarm = false;
    uint64_t score = 0;

    if ( mZones.size() <= 0 ) return( alarm );

    if ( mDebugStreams )
    {
        mCompImageSlave->relayImage( compImage );
        if ( mDebugImages )
            compImage.writeJpeg( stringtf( "%s/comp-%s-%ju.jpg", mDebugLocation.c_str(), mName.c_str(), mFrameCount ), 100 );
        mRefImageSlave->relayImage( mRefImage );
        if ( mDebugImages )
            mRefImage.writeJpeg( stringtf( "%s/ref2-%s-%ju.jpg", mDebugLocation.c_str(), mName.c_str(), mFrameCount ), 100 );
    }

    // Get the difference between the captured image and the reference image
    mDeltaCorrection = false;
    Image *deltaImage = 0;
    if ( mColourDeltas )
        deltaImage = mRefImage.delta2( compImage, mDeltaCorrection );
    else
        deltaImage = mRefImage.yDelta2( compImage, mDeltaCorrection );
    //Info( "DI: %dx%d - %d", deltaImage->width(), deltaImage->height(), deltaImage->format() );

    // Reduce the size of the diff image, also helps to despeckle
    if ( mAnalysisScale > 1 )
        deltaImage->shrink( mAnalysisScale );
    if ( mDebugStreams )
    {
        mDeltaImageSlave->relayImage( *deltaImage );
        if ( mDebugImages )
            deltaImage->writeJpeg( stringtf( "%s/delta-%s-%ju.jpg", mDebugLocation.c_str(), mName.c_str(), mFrameCount ), 100 );
    }
    // Pre-populate the variance buffer with noise
    if ( mVarBuffer.empty() )
        mVarBuffer.fill( mNoiseLevelSq, deltaImage->pixels() );

    // Blank out all exclusion zones
    for ( ZoneSet::iterator zoneIter = mZones.begin(); zoneIter!= mZones.end(); zoneIter++ )
    {
        Zone *zone = *zoneIter;
        zone->clearAlarmed();
        if ( !zone->isInactive() )
            continue;
        Debug( 3, "Blanking inactive zone %s", zone->label().c_str() );
        deltaImage->fill( RGB_BLACK, zone->getPolygon() );
    }

    // Check preclusive zones first
    for ( ZoneSet::iterator zoneIter = mZones.begin(); zoneIter!= mZones.end(); zoneIter++ )
    {
        Zone *zone = *zoneIter;
        if ( !zone->isPreclusive() )
            continue;
        Debug( 3, "Checking preclusive zone %s", zone->label().c_str() );
        if ( zone->checkMotion( deltaImage, mVarBuffer ) )
        {
            Debug( 3, "Preclusive zone %s is alarmed, zone score = %d", zone->label().c_str(), zone->score() );
            return( false );
        }
    }

    double topScore = -1.0;
    Coord alarmCentre;

    // Find all alarm pixels in active zones
    for ( ZoneSet::iterator zoneIter = mZones.begin(); zoneIter!= mZones.end(); zoneIter++ )
    {
        Zone *zone = *zoneIter;
        if ( !zone->isActive() )
            continue;
        Debug( 3, "Checking active zone %s", zone->label().c_str() );
        if ( zone->checkMotion( deltaImage, mVarBuffer ) )
        {
            alarm = true;
            score += zone->score();
            zone->setAlarmed();
            Debug( 3, "Active zone %s is alarmed, zone score = %d", zone->label().c_str(), zone->score() );
            motionZones.insert( zone );
        }
    }

    if ( alarm )
    {
        for ( ZoneSet::iterator zoneIter = mZones.begin(); zoneIter!= mZones.end(); zoneIter++ )
        {
            Zone *zone = *zoneIter;
            if ( !zone->isInclusive() )
                continue;
            Debug( 3, "Checking inclusive zone %s", zone->label().c_str() );
            if ( zone->checkMotion( deltaImage, mVarBuffer ) )
            {
                alarm = true;
                score += zone->score();
                zone->setAlarmed();
                Debug( 3, "Inclusive zone %s is alarmed, zone score = %d", zone->label().c_str(), zone->score() );
                motionZones.insert( zone );
            }
        }
    }
    else
    {
        // Find all alarm pixels in exclusive zones
        for ( ZoneSet::iterator zoneIter = mZones.begin(); zoneIter!= mZones.end(); zoneIter++ )
        {
            Zone *zone = *zoneIter;
            if ( !zone->isExclusive() )
                continue;
            Debug( 3, "Checking exclusive zone %s", zone->label().c_str() );
            if ( zone->checkMotion( deltaImage, mVarBuffer ) )
            {
                alarm = true;
                score += zone->score();
                zone->setAlarmed();
                Debug( 3, "Exclusive zone %s is alarmed, zone score = %d", zone->label().c_str(), zone->score() );
                motionZones.insert( zone );
            }
        }
    }

    if ( topScore > 0 )
    {
        Info( "Got alarm centre at %d,%d, at count %ju", alarmCentre.x(), alarmCentre.y(), mFrameCount );
    }

    // Update the variance buffer based on the delta image
    uint8_t *pDelta = deltaImage->buffer().head();
    uint32_t *pBuf = mVarBuffer.head();
    int deltaStep = deltaImage->pixelStep();
    int varBufShift = getShift( mVarBlend, mFastStart?mFrameCount+1:0 );
    int varDeltaShift = 16 - varBufShift;

    if ( deltaImage->format() == Image::FMT_GREY16 )
    {
        while( pDelta != deltaImage->buffer().tail() )
        {
            uint16_t delta = (((uint16_t)*pDelta)<<8)+*(pDelta+1);
            //uint32_t long deltaSq = delta*delta*2;
            uint64_t deltaSq = delta*delta;
            *pBuf = *pBuf - (*pBuf >> varBufShift) + (deltaSq >> varDeltaShift);
            // Add a noise threshold
            if ( *pBuf < mNoiseLevelSq )
                *pBuf = mNoiseLevelSq;
            pDelta += deltaStep;
            pBuf++;
        }
    }
    else
    {
        while( pDelta != deltaImage->buffer().tail() )
        {
            uint8_t delta = *pDelta;
            //uint16_t deltaSq = delta*delta*2;
            uint16_t deltaSq = delta*delta;
            *pBuf = *pBuf - (*pBuf >> varBufShift) + (deltaSq << varDeltaShift);
            // Add a noise threshold
            if ( *pBuf < mNoiseLevelSq )
                *pBuf = mNoiseLevelSq;
            pDelta += deltaStep;
            pBuf++;
        }
    }

    if ( mDebugStreams )
    {
        if ( !mVarImage )
            mVarImage = new Image( Image::FMT_GREY, deltaImage->width(), deltaImage->height() );
        else
            mVarImage->erase();

        uint8_t *pVar = mVarImage->buffer().head();
        pBuf = mVarBuffer.head();
        int varStep = mVarImage->pixelStep();
        while( pBuf != mVarBuffer.tail() )
        {
            *pVar = (uint8_t)sqrt(*pBuf>>16);
            pVar += varStep;
            pBuf++;
        }
        mVarImageSlave->relayImage( *mVarImage );
        if ( mDebugImages )
            mVarImage->writeJpeg( stringtf( "%s/var-%s-%ju.jpg", mDebugLocation.c_str(), mName.c_str(), mFrameCount ), 100 );
    }
    delete deltaImage;
    //mRefImage.blend( compImage, mRefBlend );

    // This is a small and innocent hack to prevent scores of 0 being returned in alarm state
    return( score?score:alarm );
}

/**
* @brief 
*
* @param frame
* @param 
*
* @return 
*/
bool MotionDetector::inAlarm( const FramePtr &frame, const FeedConsumer * )
{
    const AlarmFrame *alarmFrame = dynamic_cast<const AlarmFrame *>(frame.get());
    //const MotionDetector *motionDetector = dynamic_cast<const MotionDetector *>( videoFrame->provider() );
    return( alarmFrame->alarmed() );
}

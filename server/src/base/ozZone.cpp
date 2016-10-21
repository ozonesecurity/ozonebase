#include "oz.h"
#include "ozZone.h"

#include "../libgen/libgenUtils.h"
#include "../libimg/libimgImage.h"

#include <math.h>

/**
* @brief 
*
* @param type
* @param alarmRgb
* @param checkBlobs
* @param diffThres
* @param scoreThres
* @param scoreBlend
* @param minAlarmPercent
* @param maxAlarmPercent
* @param minScore
* @param maxScore
*/
void Zone::setup()
{
    mScore = 0;
    mAlarmed = false;

    mScaleSq = mConfig.mScale*mConfig.mScale;
    
    mImageCount = 0;

	Debug( 1, "Initialised zone %d/%s - %d - %dx%d - Rgb:%06x, cB:%d, dT:%.2f, sT:%.2f, sB:%lu, mnA%%:%.2lf, mxAP%%:%.2lf, mnSC:%lu, mxSC:%lu", mId, mIdentity.c_str(), mType, mPolygon.width(), mPolygon.height(), mConfig.mAlarmRgb, mConfig.mCheckBlobs, mConfig.mDiffThres, mConfig.mScoreThres, mConfig.mScoreBlend, mConfig.mMinAlarmPercent, mConfig.mMaxAlarmPercent, mConfig.mMinScore, mConfig.mMaxScore );

    mMask = new PolyMask( mPolygon );
    if ( mConfig.mScale > 1 )
        mMask->shrink( mConfig.mScale );

    if ( mType == ACTIVE || mType == INCLUSIVE )
        mMotionData = new MotionData( this );

    mMotionImage = NULL;
    mThresImage = NULL;
}

/**
* @brief 
*/
Zone::~Zone()
{
    delete mThresImage;
    delete mMotionData;
	delete mMask;
}

/**
* @brief 
*
* @param deltaImage
* @param varBuffer
*
* @return 
*/
bool Zone::checkMotion( const Image *deltaImage, const Uint32Buffer &varBuffer )
{
	bool alarm = false;

    if ( !mThresImage )
        mThresImage = new Image( Image::FMT_GREY, deltaImage->width(), deltaImage->height() );
    else
        mThresImage->erase();

    if ( mMotionData )
	    mMotionData->reset();

    delete mMotionImage;
    mMotionImage = NULL;

    unsigned long minAlarmPixels = (deltaImage->pixels()*mConfig.mMinAlarmPercent)/100.0L;
    unsigned long maxAlarmPixels = (deltaImage->pixels()*mConfig.mMaxAlarmPercent)/100.0L;

    const Image *maskImage = mMask->image();
    const ByteBuffer &maskBuffer = mMask->image()->buffer();

    uint8_t *pDelta = deltaImage->buffer().head();
    uint8_t *pThres = mThresImage->buffer().head();
    uint32_t *pBuf = varBuffer.head();
    const uint8_t *pMask = maskBuffer.data();

    uint64_t scoreTotal = 0;

    int thresStep = mThresImage->pixelStep();
    int deltaStep = deltaImage->pixelStep();

    int minAlarmX = deltaImage->width()-1, minAlarmY = deltaImage->height()-1, maxAlarmX = 0, maxAlarmY = 0;

    int maskLoY = mMask->loY();
    int maskHiY = mMask->hiY();
    int maskLoX = mMask->loX();
    //int maskHiX = mMask->hiX();

    mScore = 0;

    uint32_t alarmPixels = 0;
    for ( int y = maskLoY, maskY = 0; y <= maskHiY; y++, maskY++ )
    {
        int maskLineLoX = mMask->loX(maskY);
        int maskLineHiX = mMask->hiX(maskY);
        pThres = mThresImage->buffer().data() + (maskLineLoX+(y*mThresImage->width()))*thresStep;
        pDelta = deltaImage->buffer().data() + (maskLineLoX+(y*deltaImage->width()))*deltaStep;
        pBuf = varBuffer.data() + (maskLineLoX+(y*deltaImage->width()));
        pMask = maskBuffer.data() + (maskLineLoX-maskLoX) + ((y-maskLoY)*maskImage->width());
        for ( int x = maskLineLoX; x <= maskLineHiX; x++, pDelta += deltaStep, pThres += thresStep, pBuf++, pMask++ )
        {
            if ( *pMask )
            {
                unsigned char delta = *pDelta;
                unsigned short deltaSq = delta * delta;
#if 1
                unsigned char base = (unsigned char)sqrt(*pBuf>>16); // Expensive
                if ( delta > (mConfig.mDiffThres * base) )
#else
                unsigned short base = (unsigned short)(*pBuf>>16);
                if ( deltaSq > (mConfig.mDiffThres * base) )
#endif
                {
                    *pThres = 0xff;
                    //*(pThres+1) = 0;
                    //*pThres = (unsigned char)(deltaSq >> 8);
                    //*(pThres+1) = (unsigned char)deltaSq;

                    alarmPixels++;
                    // Fourth power to highlight the most changed pixels
                    //scoreTotal += deltaSq * deltaSq;
                    scoreTotal += deltaSq;
                    if ( x < minAlarmX )
                        minAlarmX = x;
                    if ( y < minAlarmY )
                        minAlarmY = y;
                    if ( x > maxAlarmX )
                        maxAlarmX = x;
                    if ( y > maxAlarmY )
                        maxAlarmY = y;
                }
            }
        }
    }
//#ifdef HAVE_SQRTL
    //uint32_t score = (uint32_t)(alarmPixels?(sqrtl(scoreTotal/alarmPixels)):0);
//#else // HAVE_SQRTL
    mScore = (uint32_t)(alarmPixels?(sqrt(scoreTotal/alarmPixels)):0);
//#endif // HAVE_SQRTL
    if ( true || mMotionData )
    {
        //if ( !motionData.refScore )
            //motionData.refScore = (unsigned long long)motionData.score<<32;
        //Debug( 3, "s:%lu,rS:%llu,sP:%lu,sT:%llu", mMotionData->score, mMotionData->refScore>>32, mMotionData->alarmPixels, scoreTotal );
        Debug( 3, "s:%u,sP:%u,sT:%ju", mScore, alarmPixels, scoreTotal );
    }

    /*
    if ( mType == INCLUSIVE )
        score >>= 1;
    else if ( mType == EXCLUSIVE )
        score <<= 1;
    */
    Debug( 5, "Adjusted score is %u", mScore );

    if ( mConfig.mCheckBlobs )
    {
        //mThresImage->despeckleFilter( 2 );
        Image::BlobGroup blobGroup;
        // Adjust for scale
        blobGroup.setFilter( minAlarmPixels, maxAlarmPixels );
        //blobGroup.minSize( mConfig.minAlarmPixels );
        //blobGroup.maxSize( mConfig.maxAlarmPixels );
        int alarmBlobs = mThresImage->locateBlobs( blobGroup, mMask, true, true );
        if ( alarmBlobs )
        {
            if ( mMotionData )
            {
                mMotionImage = mThresImage->highlightEdges( RGB_RED, mMask );
                Debug( 3, "Got %lu blobs @ %d x %d", blobGroup.blobList().size(), blobGroup.centre().x(), blobGroup.centre().y() );
                for ( int b = 0; b < blobGroup.blobList().size(); b++ )
                {
                    const Image::Blob *blob = blobGroup.blobList()[b];
                    Debug( 3, "Blob %d: %d pixels (%d x %d - %d x %d @ %d x %d )",
                        b,
                        blob->pixels(),
                        blob->loX(),
                        blob->loY(),
                        blob->hiX(),
                        blob->hiY(),
                        blob->origin().x(),
                        blob->origin().y()
                    );
                    mMotionImage->fill( RGB_YELLOW, blob->centre() );
                }
                mMotionImage->fill( RGB_GREEN, blobGroup.centre() );
                if ( mConfig.mScale > 1 )
                    mMotionImage->swell( mConfig.mScale );
                //motionImage->writeJpeg( stringtf( "/transfer/edge2-%04d.jpg", mId ), 100 );

                mMotionData->assign( blobGroup.pixels()*mScaleSq, mScore, blobGroup.extent()*mConfig.mScale, blobGroup.centre()*mConfig.mScale, blobGroup );
            }
            alarm = true;
        }
    }
    else
    {
        //if ( (minAlarmPixels && (alarmPixels >= minAlarmPixels)) && (maxAlarmPixels && (alarmPixels <= maxAlarmPixels)) && motionData.score > (mConfig.mScoreThres * (motionData.refScore>>32)) )
        if ( (!minAlarmPixels || (alarmPixels >= minAlarmPixels)) && (!maxAlarmPixels || (alarmPixels <= maxAlarmPixels)) )
        {
            if ( mMotionData )
            {
                Box extent( minAlarmX, minAlarmY, maxAlarmX, maxAlarmY );
                mMotionImage = new Image( Image::FMT_RGB, deltaImage->width(), deltaImage->height() );
                mMotionImage->outline( RGB_RED, extent );
                mMotionImage->fill( RGB_GREEN, extent.centre() );
                if ( mConfig.mScale > 1 )
                {
                    mMotionImage->swell( mConfig.mScale );
                    extent *= mConfig.mScale;
                }
                mMotionData->assign( alarmPixels*mScaleSq, mScore, extent, extent.centre() );
            }
            alarm = true;
        }
    }
    mImageCount++;
    //int scoreShift = getShift( mConfig.mScoreBlend, mImageCount );
    //motionData.refScore = motionData.refScore - (motionData.refScore >> scoreShift) + ((unsigned long long)motionData.score << (32 - scoreShift));

	return( alarm );
}

/**
* @brief 
*
* @param zone_string
* @param zone_id
* @param colour
* @param polygon
*
* @return 
*/
bool Zone::parseZoneString( const char *zone_string, int &zone_id, int &colour, Polygon &polygon )
{
	Debug( 3, "Parsing zone string '%s'", zone_string );

	char *strPtr = new char[strlen(zone_string)+1];
	char *str = strPtr;
	strcpy( str, zone_string );

	char *whiteSpace = strchr( str, ' ' );
	if ( !whiteSpace )
	{
		Debug( 3, "No whitespace found in zone string '%s', finishing", zone_string );
	}
	zone_id = strtol( str, 0, 10 );
	Debug( 3, "Got zone %d from zone string", zone_id );
	if ( !whiteSpace )
	{
		delete strPtr;
		return( true );
	}

	*whiteSpace = '\0';
	str = whiteSpace+1;

	whiteSpace = strchr( str, ' ' );
	if ( !whiteSpace )
	{
		Error( "No whitespace found in zone string '%s'", zone_string );
		delete[] strPtr;
		return( false );
	}
	*whiteSpace = '\0';
	colour = strtol( str, 0, 16 );
	Debug( 3, "Got colour %06x from zone string", colour );
	str = whiteSpace+1;

	bool result = parsePolygonString( str, polygon );

	//printf( "Area: %d\n", pg.area() );
	//printf( "Centre: %d,%d\n", pg.Centre().x(), pg.Centre().y() );

	delete[] strPtr;

	return( result );
}

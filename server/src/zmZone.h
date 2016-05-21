#ifndef ZONE_H
#define ZONE_H

#include "zmRegion.h"

#include "libimg/libimgRgb.h"
#include "libimg/libimgCoord.h"
#include "libimg/libimgPoly.h"
#include "libimg/libimgImage.h"

#include "zmMotionFrame.h"

//
// This describes a 'zone', or an area of an image that has certain
// detection characteristics.
//
class Zone : public Region
{
public:
    typedef enum { ACTIVE=1, INCLUSIVE, EXCLUSIVE, PRECLUSIVE, INACTIVE } ZoneType;

protected:
    ZoneType        mType;
    Mask            *mMask;
    Rgb             mAlarmRgb;
    bool            mCheckBlobs;

    double          mDiffThres;
    double          mScoreThres;
    unsigned long   mScoreBlend;

    int             mMinAlarmPixels;
    int             mMaxAlarmPixels;

    unsigned long   mMinScore;
    unsigned long   mMaxScore;

    int             mScale;     // How much to shrink images for analysis
    int             mScaleSq;   // The above, squared

    uint32_t        mScore;
    bool            mAlarmed;       // Current alarm state
    int             mImageCount;
    MotionData      *mMotionData;
    Image           *mMotionImage;

private:
    Image           *mThresImage;

protected:
    void setup( ZoneType type, const Rgb alarmRgb, bool checkBlobs, double diffThres, double scoreThres, unsigned long scoreBlen, int minAlarmPixels, int maxAlarmPixels, unsigned long minScore, unsigned long maxScore );

public:
    Zone( int id, const char *label, ZoneType type, const Polygon &polygon, const Rgb alarmRgb, bool checkBlobs, double diffThres=2.0, double scoreThres=1.41, unsigned long scoreBlend=64, int minAlarmPixels=100, int maxAlarmPixels=0, unsigned long minScore=50, unsigned long maxScore=0 ) :
        Region( id, label, polygon ),
        mMotionData( NULL ),
        mThresImage( NULL )
    {
        setup( type, alarmRgb, checkBlobs, diffThres, scoreThres, scoreBlend, minAlarmPixels, maxAlarmPixels, minScore, maxScore );
    }
    Zone( int id, const char *label, const Polygon &polygon, const Rgb alarmRgb, bool checkBlobs, double diffThres=2.0, double scoreThres=1.41, unsigned long scoreBlend=64, int minAlarmPixels=100, int maxAlarmPixels=0, unsigned long minScore=50, unsigned long maxScore=0 ) :
        Region( id, label, polygon ),
        mMotionData( NULL ),
        mThresImage( NULL )
    {
        setup( Zone::ACTIVE, alarmRgb, checkBlobs, diffThres, scoreThres, scoreBlend, minAlarmPixels, maxAlarmPixels, minScore, maxScore );
    }
    Zone( int id, const char *label, const Polygon &polygon ) :
        Region( id, label, polygon ),
        mMotionData( NULL ),
        mThresImage( NULL )
    {
        setup( Zone::INACTIVE, RGB_BLACK, false, 0.0, 0.0, 0, 0, 0, 0, 0 );
    }

public:
    ~Zone();

    inline ZoneType type() const { return( mType ); }
    inline bool isActive() const { return( mType == ACTIVE ); }
    inline bool isInclusive() const { return( mType == INCLUSIVE ); }
    inline bool isExclusive() const { return( mType == EXCLUSIVE ); }
    inline bool isPreclusive() const { return( mType == PRECLUSIVE ); }
    inline bool isInactive() const { return( mType == INACTIVE ); }

    inline uint32_t score() const { return( mScore ); }
    inline bool isAlarmed() const { return( mAlarmed ); }
    inline void setAlarmed() { mAlarmed = true; }
    inline void clearAlarmed() { mAlarmed = false; }

    MotionData *motionData() const { return( mMotionData ); }
    const Image *motionImage() const { return( mMotionImage ); }

    bool checkMotion( const Image *deltaImage, const Uint32Buffer &varBuffer );
    bool dumpSettings( char *output, bool verbose );

    static bool parseZoneString( const char *zoneString, int &zoneId, int &colour, Polygon &polygon );
};

#endif // ZONE_H

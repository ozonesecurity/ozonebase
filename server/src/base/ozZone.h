#ifndef ZONE_H
#define ZONE_H

#include "ozRegion.h"

#include "../libimg/libimgRgb.h"
#include "../libimg/libimgCoord.h"
#include "../libimg/libimgPoly.h"
#include "../libimg/libimgImage.h"

#include "ozMotionFrame.h"

//
// This describes a 'zone', or an area of an image that has certain
// detection characteristics.
//
class Zone : public Region
{
public:
    typedef enum { ACTIVE=1, INCLUSIVE, EXCLUSIVE, PRECLUSIVE, INACTIVE } ZoneType;

public:
    class Config
    {
    public:
        Rgb             mAlarmRgb;
        bool            mCheckBlobs;

        double          mDiffThres;
        double          mScoreThres;
        unsigned long   mScoreBlend;

        double          mMinAlarmPercent;
        double          mMaxAlarmPercent;

        unsigned long   mMinScore;
        unsigned long   mMaxScore;

        int             mScale;     // How much to shrink images for analysis

    public:
        // Set some useful defaults
        Config( const Rgb alarmRgb=RGB_RED, int scale=2, bool checkBlobs=true, double diffThres=2.0, double scoreThres=1.41, unsigned long scoreBlend=64, double minAlarmPercent=2, double maxAlarmPercent=0, unsigned long minScore=50, unsigned long maxScore=0 ) :
            mAlarmRgb( alarmRgb ),
            mCheckBlobs( checkBlobs ),
            mDiffThres( diffThres ),
            mScoreThres( scoreThres ),
            mScoreBlend( scoreBlend ),
            mMinAlarmPercent( minAlarmPercent ),
            mMaxAlarmPercent( maxAlarmPercent ),
            mMinScore( minScore ),
            mMaxScore( maxScore ),
            mScale( scale )
        {
        }
    };

protected:
    ZoneType        mType;
    Mask            *mMask;

    Config          mConfig;

    int             mScaleSq;   // mScale squared

    uint32_t        mScore;
    bool            mAlarmed;       // Current alarm state
    int             mImageCount;
    MotionData      *mMotionData;
    Image           *mMotionImage;

private:
    Image           *mThresImage;

protected:
    void setup();

public:
    Zone( int id, const char *label, ZoneType type, const Polygon &polygon, const Config &config ) :
        Region( id, label, polygon ),
        mType( type ),
        mConfig( config ),
        mMotionData( NULL ),
        mThresImage( NULL )
    {
        setup();
    }
    Zone( int id, const char *label, const Polygon &polygon, const Config &config ) :
        Region( id, label, polygon ),
        mType( ACTIVE ),
        mConfig( config ),
        mMotionData( NULL ),
        mThresImage( NULL )
    {
        setup();
    }
    Zone( int id, const char *label, const Polygon &polygon ) :
        Region( id, label, polygon ),
        mType( INACTIVE ),
        mConfig( Config( RGB_BLACK, 1, false, 0.0, 0.0, 0, 0, 0, 0, 0 ) ),
        mMotionData( NULL ),
        mThresImage( NULL )
    {
        setup();
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

#ifndef ZM_MOTION_DATA_H
#define ZM_MOTION_DATA_H

#include "libimg/libimgBox.h"
#include "libimg/libimgCoord.h"
#include "libimg/libimgImage.h"

class Region;

class MotionData
{
private:
    Region      *mRegion;
    uint32_t    mScore;
    uint32_t    mPixels;
    Box         mExtent;
    Coord       mCentre;
    Image::BlobGroup mBlobGroup;

public:
    MotionData() :
        mRegion( NULL ),
        mScore( 0 ),
        mPixels( 0 )
    {
    }
    MotionData( Region *region ) :
        mRegion( region ),
        mScore( 0 ),
        mPixels( 0 )
    {
    }
    MotionData( const MotionData &motionData ) :
        mRegion( motionData.mRegion ),
        mScore( motionData.mScore ),
        mPixels( motionData.mPixels ),
        mExtent( motionData.mExtent ),
        mCentre( motionData.mCentre ),
        mBlobGroup( motionData.mBlobGroup )
    {
    }
    ~MotionData()
    {
    }
    MotionData *clone() const
    {
        return( new MotionData( *this ) );
    }
    inline void reset()
    {
        mScore = 0;
        mPixels = 0;
    }
    inline void assign( uint32_t score, uint32_t pixels, const Box &extent, const Coord &centre )
    {
        mScore = score;
        mPixels = pixels;
        mExtent = extent;
        mCentre = centre;
    }
    inline void assign( uint32_t score, uint32_t pixels, const Box &extent, const Coord &centre, const Image::BlobGroup &blobGroup )
    {
        mScore = score;
        mPixels = pixels;
        mExtent = extent;
        mCentre = centre;
        mBlobGroup = blobGroup;
    }

    uint32_t pixels() const { return( mPixels ); }
    const Box &extent() const { return( mExtent ); }
    const Coord &centre() const { return( mCentre ); }
    uint32_t score() const { return( mScore ); }
    Region *region() const { return( mRegion ); }

    MotionData &operator+=( const MotionData &motionData )
    {
        if ( mRegion != motionData.mRegion )
            mRegion = NULL;
        mScore += motionData.mScore;
        mPixels += motionData.mPixels;
        mExtent += motionData.mExtent;
        mCentre = mExtent.centre();
        mBlobGroup += motionData.mBlobGroup;
        return( *this );
    }
    friend MotionData operator+( const MotionData &motionData1, const MotionData &motionData2 )
    {
        MotionData result( motionData1 );
        result += motionData2;
        return( result );
    }
};

#endif // ZM_MOTION_DATA_H

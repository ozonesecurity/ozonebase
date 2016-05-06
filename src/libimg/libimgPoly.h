#ifndef LIBIMG_POLY_H
#define LIBIMG_POLY_H

#include "libimgCoord.h"
#include "libimgBox.h"

#include <math.h>

//
// Class used for storing a box, which is defined as a region
// defined by two coordinates
//
class Polygon
{
protected:
    int mNumCoords;
    Coord *mCoords;
    Box mExtent;
    int mArea;
    Coord mCentre;

protected:
    void initialiseEdges();
    void calcArea();
    void calcCentre();

public:
    Polygon() : mNumCoords( 0 ), mCoords( 0 ), mArea( 0 )
    {
    }
    Polygon( int numCoords, const Coord *coords );
    Polygon( const Polygon &polygon );
    ~Polygon()
    {
        delete[] mCoords;
    }

    Polygon &operator=( const Polygon &polygon );

    int numCoords() const { return( mNumCoords ); }
    const Coord &coord( int index ) const
    {
        return( mCoords[index] );
    }

    const Box &extent() const { return( mExtent ); }
    int loX() const { return( mExtent.loX() ); }
    int hiX() const { return( mExtent.hiX() ); }
    int loY() const { return( mExtent.loY() ); }
    int hiY() const { return( mExtent.hiY() ); }
    int width() const { return( mExtent.width() ); }
    inline int height() const { return( mExtent.height() ); }

    int area() const { return( mArea ); }
    const Coord &centre() const
    {
        return( mCentre );
    }
    bool isInside( const Coord &coord ) const;
};

#endif // LIBIMG_POLY_H

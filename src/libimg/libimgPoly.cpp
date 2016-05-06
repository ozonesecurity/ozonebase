#include "libimgPoly.h"

#include <math.h>

void Polygon::calcArea()
{
    double floatArea = 0.0L;
    for ( int i = 0, j = mNumCoords-1; i < mNumCoords; j = i++ )
    {
        double trapArea = ((mCoords[i].x()-mCoords[j].x())*((mCoords[i].y()+mCoords[j].y())))/2.0L;
        floatArea += trapArea;
        //printf( "%.2f (%.2f)\n", floatArea, trapArea );
    }
    mArea = (int)round(fabs(floatArea));
}

void Polygon::calcCentre()
{
    if ( !mArea && mNumCoords )
        calcArea();
    double floatX = 0.0L, floatY = 0.0L;
    for ( int i = 0, j = mNumCoords-1; i < mNumCoords; j = i++ )
    {
        floatX += ((mCoords[i].y()-mCoords[j].y())*((mCoords[i].x()*2)+(mCoords[i].x()*mCoords[j].x())+(mCoords[j].x()*2)));
        floatY += ((mCoords[j].x()-mCoords[i].x())*((mCoords[i].y()*2)+(mCoords[i].y()*mCoords[j].y())+(mCoords[j].y()*2)));
    }
    floatX /= (6*mArea);
    floatY /= (6*mArea);
    //printf( "%.2f,%.2f\n", floatX, floatY );
    mCentre = Coord( (int)round(floatX), (int)round(floatY) );
}

Polygon::Polygon( int numCoords, const Coord *coords ) : mNumCoords( numCoords )
{
    mCoords = new Coord[mNumCoords];

    int minX = -1;
    int maxX = -1;
    int minY = -1;
    int maxY = -1;
    for( int i = 0; i < mNumCoords; i++ )
    {
        mCoords[i] = coords[i];
        if ( minX == -1 || mCoords[i].x() < minX )
            minX = mCoords[i].x();
        if ( maxX == -1 || mCoords[i].x() > maxX )
            maxX = mCoords[i].x();
        if ( minY == -1 || mCoords[i].y() < minY )
            minY = mCoords[i].y();
        if ( maxY == -1 || mCoords[i].y() > maxY )
            maxY = mCoords[i].y();
    }
    mExtent = Box( minX, minY, maxX, maxY );
    calcArea();
    calcCentre();
}

Polygon::Polygon( const Polygon &polygon ) : mNumCoords( polygon.mNumCoords ), mExtent( polygon.mExtent ), mArea( polygon.mArea ), mCentre( polygon.mCentre )
{
    mCoords = new Coord[mNumCoords];
    for( int i = 0; i < mNumCoords; i++ )
    {
        mCoords[i] = polygon.mCoords[i];
    }
}

Polygon &Polygon::operator=( const Polygon &polygon )
{
    if ( mNumCoords < polygon.mNumCoords )
    {
        delete[] mCoords;
        mCoords = new Coord[polygon.mNumCoords];
    }
    mNumCoords = polygon.mNumCoords;
    for( int i = 0; i < mNumCoords; i++ )
    {
        mCoords[i] = polygon.mCoords[i];
    }
    mExtent = polygon.mExtent;
    mArea = polygon.mArea;
    mCentre = polygon.mCentre;
    return( *this );
}

bool Polygon::isInside( const Coord &coord ) const
{
    bool inside = false;
    for ( int i = 0, j = mNumCoords-1; i < mNumCoords; j = i++ )
    {
        if ( (((mCoords[i].y() <= coord.y()) && (coord.y() < mCoords[j].y()) )
        || ((mCoords[j].y() <= coord.y()) && (coord.y() < mCoords[i].y())))
        && (coord.x() < (mCoords[j].x() - mCoords[i].x()) * (coord.y() - mCoords[i].y()) / (mCoords[j].y() - mCoords[i].y()) + mCoords[i].x()))
        {
            inside = !inside;
        }
    }
    return( inside );
}

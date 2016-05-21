#ifndef LIBIMG_BOX_H
#define LIBIMG_BOX_H

#include "libimgCoord.h"

#include <math.h>

//
// Class used for storing a box, which is defined as a region
// defined by two coordinates
//
class Box
{
private:
    Coord mLo, mHi;
    Coord mSize;

public:
    Box()
    {
    }
    Box( const Box &box ) : mLo( box.mLo ), mHi( box.mHi ), mSize( box.mSize )
    {
    }
    Box( int size ) : mLo( 0, 0 ), mHi ( size-1, size-1 ), mSize( Coord::range( mHi, mLo ) ) { }
    Box( int x_size, int y_size ) : mLo( 0, 0 ), mHi ( x_size-1, y_size-1 ), mSize( Coord::range( mHi, mLo ) ) { }
    Box( int loX, int loY, int hiX, int hiY ) : mLo( loX, loY ), mHi( hiX, hiY ), mSize( Coord::range( mHi, mLo ) ) { }
    Box( const Coord &lo, const Coord &hi ) : mLo( lo ), mHi( hi ), mSize( Coord::range( mHi, mLo ) ) { }

    const Coord &lo() const { return( mLo ); }
    int loX() const { return( mLo.x() ); }
    int loY() const { return( mLo.y() ); }
    const Coord &hi() const { return( mHi ); }
    int hiX() const { return( mHi.x() ); }
    int hiY() const { return( mHi.y() ); }
    const Coord &size() const { return( mSize ); }
    int width() const { return( mSize.x() ); }
    inline int height() const { return( mSize.y() ); }
    int area() const { return( mSize.x()*mSize.y() ); }

    const Coord centre() const
    {
        int midX = int(round(mLo.x()+(mHi.x()/2.0)));
        int midY = int(round(mLo.y()+(mHi.y()/2.0)));
        return( Coord( midX, midY ) );
    }
    bool inside( const Coord &coord ) const
    {
        return( coord.x() >= mLo.x() && coord.x() <= mHi.x() && coord.y() >= mLo.y() && coord.y() <= mHi.y() );
    }

    Box &operator=( const Box &box )
    {
        mLo = box.mLo;
        mHi = box.mHi;
        mSize = box.mSize;
        return( *this );
    }
    Box &operator+=( const Box &box )
    {
        mLo = Coord::min(mLo,box.mLo);
        mHi = Coord::max(mHi,box.mHi);
        mSize = Coord::range( mHi, mLo );
        return( *this );
    }
    Box &operator*=( int factor ) { mLo *= factor; mHi *= factor; mSize *= factor; return( *this ); }
    friend Box operator+( const Box &box1, const Box &box2 ) { Box result( box1 ); result += box2; return( result ); }
    friend Box operator*( const Box &box, int factor ) { Box result( box ); result *= factor; return( result ); }
};

#endif // LIBIMG_BOX_H

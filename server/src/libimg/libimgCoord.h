#ifndef LIBIMG_COORD_H
#define LIBIMG_COORD_H

//
// Class used for storing an x,y pair, i.e. a coordinate
//
class Coord
{
private:
    int mX, mY;

public:
    Coord() : mX(0), mY(0)
    {
    }
    Coord( int x, int y ) : mX(x), mY(y)
    {
    }
    Coord( const Coord &coord ) : mX(coord.mX), mY(coord.mY)
    {
    }
    inline int &x() { return( mX ); }
    inline const int &x() const { return( mX ); }
    inline int &y() { return( mY ); }
    inline const int &y() const { return( mY ); }

    static Coord range( const Coord &coord1, const Coord &coord2 )
    {
        Coord result( (coord1.mX-coord2.mX)+1, (coord1.mY-coord2.mY)+1 );
        return( result );
    }
    static Coord max( const Coord &coord1, const Coord &coord2 )
    {
        Coord result( coord1.mX>coord2.mX?coord1.mX:coord2.mX, coord1.mY>coord2.mY?coord1.mY:coord2.mY );
        return( result );
    }
    static Coord min( const Coord &coord1, const Coord &coord2 )
    {
        Coord result( coord1.mX<coord2.mX?coord1.mX:coord2.mX, coord1.mY<coord2.mY?coord1.mY:coord2.mY );
        return( result );
    }

    Coord &operator=( const Coord &coord ) { mX = coord.mX; mY = coord.mY; return( *this ); }

    bool operator==( const Coord &coord ) { return( mX == coord.mX && mY == coord.mY ); }
    bool operator!=( const Coord &coord ) { return( mX != coord.mX || mY != coord.mY ); }
    bool operator>( const Coord &coord ) { return( mX > coord.mX && mY > coord.mY ); }
    bool operator>=( const Coord &coord ) { return( !(operator<(coord)) ); }
    bool operator<( const Coord &coord ) { return( mX < coord.mX && mY < coord.mY ); }
    bool operator<=( const Coord &coord ) { return( !(operator>(coord)) ); }
    Coord &operator+=( const Coord &coord ) { mX += coord.mX; mY += coord.mY; return( *this ); }
    Coord &operator-=( const Coord &coord ) { mX -= coord.mX; mY -= coord.mY; return( *this ); }
    Coord &operator*=( int factor ) { mX *= factor; mY *= factor; return( *this ); }

    friend Coord operator+( const Coord &coord1, const Coord &coord2 ) { Coord result( coord1 ); result += coord2; return( result ); }
    friend Coord operator-( const Coord &coord1, const Coord &coord2 ) { Coord result( coord1 ); result -= coord2; return( result ); }
    friend Coord operator*( const Coord &coord1, int factor ) { Coord result( coord1 ); result *= factor; return( result ); }
};

#endif // LIBIMG_COORD_H

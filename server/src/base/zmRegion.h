#ifndef REGION_H
#define REGION_H

#include "../libimg/libimgPoly.h"
#include "../libimg/libimgImage.h"

//
// This describes a 'zone', or an area of an image that has certain
// detection characteristics.
//
class Region
{
protected:
    struct Range
    {
        int loX;
        int hiX;
        int offX;
    };

protected:
    int             mId;
    std::string     mIdentity;
    Polygon         mPolygon;
    Mask            *mMask;

public:
    Region( int id, const char *label, const Polygon &polygon );

public:
    ~Region();

    inline int id() const { return( mId ); }
    inline const std::string &label() const { return( mIdentity ); }
    inline const Polygon &getPolygon() const { return( mPolygon ); }

    static bool parsePolygonString( const char *polygonString, Polygon &polygon );
};

#endif // REGION_H

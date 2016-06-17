#include "oz.h"
#include "ozRegion.h"

//#include "../libgen/libgenUtils.h"
#include "../libimg/libimgImage.h"

//#include <math.h>

/**
* @brief 
*
* @param id
* @param label
* @param polygon
*/
Region::Region( int id, const char *label, const Polygon &polygon ) :
    mId( id ),
    mIdentity( label ),
    mPolygon( polygon )
{
	Debug( 1, "Initialised region %d/%s %dx%d", mId, mIdentity.c_str(), mPolygon.width(), mPolygon.height() );
}

/**
* @brief 
*/
Region::~Region()
{
	delete mMask;
}

/**
* @brief 
*
* @param polyString
* @param polygon
*
* @return 
*/
bool Region::parsePolygonString( const char *polyString, Polygon &polygon )
{
	Debug( 3, "Parsing polygon string '%s'", polyString );

	char *strPtr = new char[strlen(polyString)+1];
	char *str = strPtr;
	strcpy( str, polyString );

	char *whiteSpace;
	int numCoords = 0;
	int maxNumCoords = strlen(str)/4;
	Coord *coords = new Coord[maxNumCoords];
	while( true )
	{
		if ( *str == '\0' )
		{
			break;
		}
		whiteSpace = strchr( str, ' ' );
		if ( whiteSpace )
		{
			*whiteSpace = '\0';
		}
		char *comma = strchr( str, ',' );
		if ( !comma )
		{
			Error( "Bogus coordinate %s found in polygon string", str );
			delete[] coords;
			delete[] strPtr;
			return( false );
		}
		else
		{
			*comma = '\0';
			char *xp = str;
			char *yp = comma+1;

			int x = atoi(xp);
			int y = atoi(yp);

			Debug( 3, "Got coordinate %d,%d from polygon string", x, y );
#if 0
			if ( x < 0 )
				x = 0;
			else if ( x >= width )
				x = width-1;
			if ( y < 0 )
				y = 0;
			else if ( y >= height )
				y = height-1;
#endif
			coords[numCoords++] = Coord( x, y );
		}
		if ( whiteSpace )
			str = whiteSpace+1;
		else
			break;
	}
	polygon = Polygon( numCoords, coords );

	Debug( 3, "Successfully parsed polygon string" );
	//printf( "Area: %d\n", pg.area() );
	//printf( "Centre: %d,%d\n", pg.Centre().x(), pg.Centre().y() );

	delete[] coords;
	delete[] strPtr;

	return( true );
}

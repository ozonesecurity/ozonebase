#include "../../config.h"

#include "libimgImage.h"

#include "libimgFont.h"
#include "../libgen/libgenUtils.h"
#include "../libgen/libgenException.h"

#ifdef HAVE_LINUX_VIDEODEV2_H
#include <linux/videodev2.h>
#endif // HAVE_LINUX_VIDEODEV2_H

#include <sys/stat.h>
#include <errno.h>

#include <memory>

#define ABSDIFF(a,b)    (((a)<(b))?((b)-(a)):((a)-(b)))

/**
* @brief 
*/
Mask::~Mask()
{
    delete mMaskImage;
    mMaskImage = 0;
    delete[] mRanges;
    mRanges = 0;
}

/**
* @brief 
*
* @return 
*/
const Image *Mask::image() const
{
    if ( !mMaskImage )
        const_cast<Mask *>(this)->createMaskImage();
    return( mMaskImage );
}

/**
* @brief 
*
* @param box
*/
BoxMask::BoxMask( const Box &box ) : mBox( box )
{
    mRanges = new Range[mBox.height()];
    for ( int y = 0; y < mBox.height(); y++ )
    {
        mRanges[y].loX = mBox.loX();
        mRanges[y].hiX = mBox.hiX();
        mRanges[y].offX = 0;
    }
}

/**
* @brief 
*
* @param mask
*/
BoxMask::BoxMask( const BoxMask &mask ) : Mask( mask ), mBox( mask.mBox )
{
    mRanges = new Range[mBox.height()];
    memcpy( mRanges, mask.mRanges, mBox.height()*sizeof(*mRanges) );
    if ( mask.mMaskImage )
        mMaskImage = new Image( *mask.mMaskImage );
}

/**
* @brief 
*/
void BoxMask::createMaskImage()
{
    mMaskImage = new Image( Image::FMT_GREY, mBox.hiX()+1, mBox.hiY()+1 );
    mMaskImage->fill( RGB_WHITE, mBox );
    //mMaskImage->outline( RGB_WHITE, mBox );
    if ( mBox.loX() > 0 || mBox.loY() > 0 )
        mMaskImage->crop( mBox.loX(), mBox.loY(), mBox.hiX(), mBox.hiY() );
}

/**
* @brief 
*
* @return 
*/
unsigned long BoxMask::pixels() const
{
    return( mBox.area() );
}

/**
* @brief 
*
* @param factor
*/
void BoxMask::shrink( unsigned char factor )
{
    Box newBox( mBox.loX()/factor, mBox.loY()/factor, mBox.hiX()/factor, mBox.hiY()/factor );
    *this = BoxMask( newBox );
}

/**
* @brief 
*
* @param factor
*/
void BoxMask::swell( unsigned char factor )
{
    Box newBox( mBox.loX()*factor, mBox.loY()*factor, mBox.hiX()*factor, mBox.hiY()*factor );
    *this = BoxMask( newBox );
}

/**
* @brief 
*
* @param mask
*
* @return 
*/
BoxMask &BoxMask::operator=( const BoxMask &mask )
{
    mBox = mask.mBox;
    memcpy( mRanges, mask.mRanges, mBox.height()*sizeof(*mRanges) );
    if ( mask.mMaskImage )
        mMaskImage = new Image( *mask.mMaskImage );
    return( *this );
}

/**
* @brief 
*
* @param poly
*/
PolyMask::PolyMask( const Polygon &poly ) : mPoly( poly )
{
    mRanges = new Range[mPoly.height()];
    createMaskImage();
    int y = mPoly.loY();
    for ( int polyY = 0; polyY < mPoly.height(); polyY++, y++ )
    {
        int x = mPoly.loX();
        mRanges[polyY].loX = -1;
        mRanges[polyY].hiX = -1;
        mRanges[polyY].offX = 0;
        unsigned char *pImg = mMaskImage->buffer().data()+(polyY*mMaskImage->stride());
        for ( int px = 0; px < mPoly.width(); px++, x++, pImg++ )
        {
            if ( *pImg )
            {
                if ( mRanges[polyY].loX == -1 )
                {
                    mRanges[polyY].loX = x;
                    mRanges[polyY].offX = px;
                }
                else if ( mRanges[polyY].hiX < x )
                {
                    mRanges[polyY].hiX = x;
                }
            }
        }
        //printf( "R%d: %d-%d (%d)\n", polyY, mRanges[polyY].loX, mRanges[polyY].hiX, mRanges[polyY].offX );
    }
}

/**
* @brief 
*
* @param mask
*/
PolyMask::PolyMask( const PolyMask &mask ) : Mask( mask ), mPoly( mask.mPoly )
{
    mRanges = new Range[mPoly.height()];
    memcpy( mRanges, mask.mRanges, mPoly.height()*sizeof(*mRanges) );
    if ( mask.mMaskImage )
        mMaskImage = new Image( *mask.mMaskImage );
}

/**
* @brief 
*/
void PolyMask::createMaskImage()
{
    mMaskImage = new Image( Image::FMT_GREY, mPoly.hiX()+1, mPoly.hiY()+1 );
    mMaskImage->fill( RGB_WHITE, mPoly );
    mMaskImage->outline( RGB_WHITE, mPoly );
    if ( mPoly.loX() > 0 || mPoly.loY() > 0 )
        mMaskImage->crop( mPoly.loX(), mPoly.loY(), mPoly.hiX(), mPoly.hiY() );
}

/**
* @brief 
*
* @return 
*/
unsigned long PolyMask::pixels() const
{
    return( mPoly.area() );
}

/**
* @brief 
*
* @param factor
*/
void PolyMask::shrink( unsigned char factor )
{
    Coord *newCoords = new Coord[mPoly.numCoords()];
    for ( int i = 0; i < mPoly.numCoords(); i++ )
        newCoords[i] = Coord( mPoly.coord(i).x()/factor, mPoly.coord(i).y()/factor );
    Polygon newPoly( mPoly.numCoords(), newCoords );
    *this = PolyMask( newPoly );
}

/**
* @brief 
*
* @param factor
*/
void PolyMask::swell( unsigned char factor )
{
    Coord *newCoords = new Coord[mPoly.numCoords()];
    for ( int i = 0; i < mPoly.numCoords(); i++ )
        newCoords[i] = Coord( mPoly.coord(i).x()*factor, mPoly.coord(i).y()*factor );
    Polygon newPoly( mPoly.numCoords(), newCoords );
    *this = PolyMask( newPoly );
}

/**
* @brief 
*
* @param mask
*
* @return 
*/
PolyMask &PolyMask::operator=( const PolyMask &mask )
{
    mPoly = mask.mPoly;
    memcpy( mRanges, mask.mRanges, mPoly.height()*sizeof(*mRanges) );
    if ( mask.mMaskImage )
        mMaskImage = new Image( *mask.mMaskImage );
    return( *this );
}

/**
* @brief 
*
* @param image
*/
ImageMask::ImageMask( const Image &image ) : mArea( 0 )
{
    mMaskImage = new Image( Image::FMT_GREY, image );
    mRanges = new Range[mMaskImage->height()];
    const unsigned char *pImg = mMaskImage->buffer().data();
    unsigned long minX = -1, minY = -1;
    unsigned long maxX = 0, maxY = 0;
    for ( int y = 0; y < mMaskImage->height(); y++ )
    {
        mRanges[y].loX = -1;
        mRanges[y].hiX = -1;
        mRanges[y].offX = 0;
        for ( int x = 0; x < mMaskImage->width(); x++, pImg++ )
        {
            if ( *pImg )
            {
                if ( mRanges[y].loX == -1 )
                {
                    mRanges[y].loX = x;
                    mRanges[y].offX = x;
                    if ( x < minX )
                        minX = x;
                    if ( y < minY )
                        minY = y;
                    if ( y > maxY )
                        maxY = y;
                }
                else if ( mRanges[y].hiX < x )
                {
                    mRanges[y].hiX = x;
                    if ( x > maxX )
                        maxX = x;
                }
                mArea++;
            }
        }
    }
    mBox = Box( minX, minY, maxX, maxY );
    if ( mBox.loX() > 0 || mBox.loY() > 0 )
        mMaskImage->crop( mBox.loX(), mBox.loY(), mBox.hiX(), mBox.hiY() );
}

/**
* @brief 
*
* @param mask
*/
ImageMask::ImageMask( const ImageMask &mask ) : Mask( mask ), mBox( mask.mBox ), mArea( mask.mArea )
{
    mMaskImage = new Image( *mask.mMaskImage );
    mRanges = new Range[mBox.height()];
    memcpy( mRanges, mask.mRanges, mBox.height()*sizeof(*mRanges) );
    mBox = mask.mBox;
    mArea = mask.mArea;
}

/**
* @brief 
*
* @return 
*/
unsigned long ImageMask::pixels() const
{
    return( mArea );
}

/**
* @brief 
*
* @param factor
*/
void ImageMask::shrink( unsigned char factor )
{
    Image *newImage = new Image( Image::FMT_GREY, mMaskImage->width()/factor, mMaskImage->height()/factor );
    Range *newRanges = new Range[newImage->height()];
    int samplePixels = factor * factor;
    const unsigned char *pSrc = mMaskImage->buffer().data();
    unsigned char *pDst = newImage->buffer().data();
    mArea = 0;
    for ( int y = 0; y <= mMaskImage->height()-factor; y += factor )
    {
        newRanges[y].loX = -1;
        newRanges[y].hiX = -1;
        newRanges[y].offX = 0;
        for ( int x = 0; x <= mMaskImage->width()-factor; x += factor )
        {
            unsigned long total = 0;
            for ( int dY = 0; dY < factor; dY++ )
            {
                for ( int dX = 0; dX < factor; dX++ )
                {
                    total += *(pSrc + dX + (mMaskImage->stride() * dY));
                }
            }
            pSrc += factor;
            if ( total/samplePixels > 0x80 )
            {
                *pDst++ = WHITE;
                if ( newRanges[y].loX == -1 )
                {
                    newRanges[y].loX = x;
                    newRanges[y].offX = x;
                }
                else if ( newRanges[y].hiX < x )
                {
                    newRanges[y].hiX = x;
                }
            }
            else
                *pDst++ = BLACK;
            mArea++;
        }
        pSrc += (factor-1)*mMaskImage->stride();
    }
    delete[] mRanges;
    mRanges = newRanges;
    delete mMaskImage;
    mMaskImage = newImage;
    mBox = Box( mBox.loX()/factor, mBox.loY()/factor, mBox.hiX()/factor, mBox.hiY()/factor );
}

/**
* @brief 
*
* @param factor
*/
void ImageMask::swell( unsigned char factor )
{
    Image *newImage = new Image( Image::FMT_GREY, mMaskImage->width()*factor, mMaskImage->height()*factor );
    Range *newRanges = new Range[newImage->height()];
    const unsigned char *pSrc = mMaskImage->buffer().data();
    unsigned char *pDst = newImage->buffer().data();
    for ( int y = 0; y < mMaskImage->height(); y++ )
    {
        newRanges[y*factor].loX = mRanges[y].loX;
        newRanges[y*factor].hiX = mRanges[y].hiX;
        newRanges[y*factor].offX = mRanges[y].offX;
        newRanges[(y*factor)+1].loX = mRanges[y].loX;
        newRanges[(y*factor)+1].hiX = mRanges[y].hiX;
        newRanges[(y*factor)+1].offX = mRanges[y].offX;
        for ( int x = 0; x < mMaskImage->width(); x++ )
        {
            for ( int dX = 0; dX < factor; dX++ )
            {
                *pDst = *pSrc;
                pDst++;
            }
            pSrc++;
        }
        memcpy( pDst, pDst-newImage->stride(), newImage->stride() );
        pDst += newImage->stride();
    }
    delete[] mRanges;
    mRanges = newRanges;
    delete mMaskImage;
    mMaskImage = newImage;
    mBox = Box( mBox.loX()*factor, mBox.loY()*factor, mBox.hiX()*factor, mBox.hiY()*factor );
    mArea *= factor;
}

/**
* @brief 
*
* @param mask
*
* @return 
*/
ImageMask &ImageMask::operator=( const ImageMask &mask )
{
    mMaskImage = new Image( *mask.mMaskImage );
    mRanges = new Range[mBox.height()];
    memcpy( mRanges, mask.mRanges, mBox.height()*sizeof(*mRanges) );
    return( *this );
}

bool Image::smInitialised = false;

Image::BlendTablePtr Image::smBlendTables[101];

Image::Lookups Image::smLookups;

Image::RGB2YUV Image::smRGB2YUV = {
    Image::RGB2YUV_Y,
    Image::RGB2YUV_U,
    Image::RGB2YUV_V,
};

Image::RGB2YUV Image::smRGB2YUVJ = {
    Image::RGB2YUVJ_Y,
    Image::RGB2YUVJ_U,
    Image::RGB2YUVJ_V,
};

Image::YUV2RGB Image::smYUV2RGB = {
    Image::YUV2RGB_R,
    Image::YUV2RGB_G,
    Image::YUV2RGB_B,
};

Image::YUV2RGB Image::smYUVJ2RGB = {
    Image::YUVJ2RGB_R,
    Image::YUVJ2RGB_G,
    Image::YUVJ2RGB_B,
};

jpeg_compress_struct *Image::smJpegCoCinfo[101] = { 0 };
jpeg_decompress_struct *Image::smJpegDeCinfo = 0;
local_error_mgr Image::smJpegCoError;
local_error_mgr Image::smJpegDeError;
Mutex Image::smJpegCoMutex;
Mutex Image::smJpegDeMutex;

/**
* @brief 
*/
void Image::initialise()
{
    static Mutex initMutex;

    initMutex.lock();
    smInitialised = true;

    memset( &smJpegCoError, 0, sizeof(smJpegCoError) );
    memset( &smJpegDeError, 0, sizeof(smJpegDeError) );

    // Blend tables for opacity
    for ( int i = 0; i <= 100; i++ )
        smBlendTables[i] = 0;

    Debug( 3, "Setting up static colour tables" );

    //
    // ================================================================================
    // Convert between normalized YUV ranges and YUVJ full ranges
    // ================================================================================
    //

    // Lookup table to convert Y channel from YUV range (16-235) to YUVJ range (0-255)
    smLookups.YUV2YUVJ_Y = new unsigned char[256];
    for ( int i = 0; i <= 255; i++ )
    {
        if ( i < 16 )
            smLookups.YUV2YUVJ_Y[i] = 0;
        else if ( i > 235 )
            smLookups.YUV2YUVJ_Y[i] = 255;
        else
            smLookups.YUV2YUVJ_Y[i] = clamp((255*(i-16))/219,0,255);
    }

    // Lookup table to convert Y channel from YUVJ range (0-255) to YUV range (16-235)
    smLookups.YUVJ2YUV_Y = new unsigned char[256];
    for ( int i = 0; i <= 255; i++ )
    {
        smLookups.YUVJ2YUV_Y[i] = clamp(((i*219)/255)+16,16,235);
    }

    // Lookup table to convert U/V channel from YUV range (-112-112) to YUVJ range (-127-+127)
    smLookups.YUV2YUVJ_UV = new unsigned char[256];
    for ( int i = 0; i < 256; i++ )
    {
        if ( i < 16 )
            smLookups.YUV2YUVJ_UV[i] = 16;
        else if ( i > 240 )
            smLookups.YUV2YUVJ_UV[i] = 240;
        else
            smLookups.YUV2YUVJ_UV[i] = 128 + clamp((127*(i-128))/112,-128,127);
    }

    // Lookup table to convert U/V channel from YUVJ range (-127-+127) to YUV range (-112-112)
    smLookups.YUVJ2YUV_UV = new unsigned char[256];
    for ( int i = 0; i < 256; i++ )
    {
        smLookups.YUVJ2YUV_UV[i] = 128 + clamp((((i-128)*112)/127),-112,112);
    }

    //
    // ================================================================================
    // YUVJ ranges (0-255, 0-255) to/from RGB
    // ================================================================================
    //

    // Create lookup for YUV components of RGB channels, full range YUVJ
    smLookups.RGB2YUVJ_YR = new signed short[256];
    smLookups.RGB2YUVJ_YG = new signed short[256];
    smLookups.RGB2YUVJ_YB = new signed short[256];
    smLookups.RGB2YUVJ_UR = new signed short[256];
    smLookups.RGB2YUVJ_UG = new signed short[256];
    smLookups.RGB2YUVJ_UB = new signed short[256];
    smLookups.RGB2YUVJ_VR = new signed short[256];
    smLookups.RGB2YUVJ_VG = new signed short[256];
    smLookups.RGB2YUVJ_VB = new signed short[256];
    for ( int i = 0; i <= 255; i++ )
    {
        smLookups.RGB2YUVJ_YR[i] = (299*i)/1000;
        smLookups.RGB2YUVJ_YG[i] = (587*i)/1000;
        smLookups.RGB2YUVJ_YB[i] = (114*i)/1000;

        smLookups.RGB2YUVJ_UR[i] = (-169*i)/1000;
        smLookups.RGB2YUVJ_UG[i] = (-331*i)/1000;
        smLookups.RGB2YUVJ_UB[i] = (500*i)/1000;

        smLookups.RGB2YUVJ_VR[i] = (500*i)/1000;
        smLookups.RGB2YUVJ_VG[i] = (-419*i)/1000;
        smLookups.RGB2YUVJ_VB[i] = (-81*i)/1000;
    }

    //
    // Tables to convert YUV (full range) components to RGB components
    //
    smLookups.YUVJ2RGB_RV = new signed short[256];
    smLookups.YUVJ2RGB_GU = new signed short[256];
    smLookups.YUVJ2RGB_GV = new signed short[256];
    smLookups.YUVJ2RGB_BU = new signed short[256];
    for ( int i = 0; i < 256; i++ )
    {
        smLookups.YUVJ2RGB_RV[i] = (1402*(i-128))/1000;
        smLookups.YUVJ2RGB_GU[i] = (-344*(i-128))/1000;
        smLookups.YUVJ2RGB_GV[i] = (-714*(i-128))/1000;
        // See JFIF clarification at http://www.fourcc.org/fccyvrgb.php
        //smLookups.UVJ2RGB_G_U[i] = 128 + (34414*((i-128)-128))/100000;
        //smLookups.UVJ2RGB_G_V[i] = 128 + (71414*((i-128)-128))/100000;
        smLookups.YUVJ2RGB_BU[i] = (1772*(i-128))/1000;
    }

    //
    // ================================================================================
    // YUV ranges (16-235, 16-239) to/from RGB
    // ================================================================================
    //

    // Create lookup for YUV components of RGB channels
    smLookups.RGB2YUV_YR = new signed short[256];
    smLookups.RGB2YUV_YG = new signed short[256];
    smLookups.RGB2YUV_YB = new signed short[256];
    smLookups.RGB2YUV_UR = new signed short[256];
    smLookups.RGB2YUV_UG = new signed short[256];
    smLookups.RGB2YUV_UB = new signed short[256];
    smLookups.RGB2YUV_VR = new signed short[256];
    smLookups.RGB2YUV_VG = new signed short[256];
    smLookups.RGB2YUV_VB = new signed short[256];
    for ( int i = 0; i <= 255; i++ )
    {
        smLookups.RGB2YUV_YR[i] = (257*i)/1000;
        smLookups.RGB2YUV_YG[i] = (504*i)/1000;
        smLookups.RGB2YUV_YB[i] = (98*i)/1000;

        smLookups.RGB2YUV_UR[i] = (-148*i)/1000;
        smLookups.RGB2YUV_UG[i] = (-291*i)/1000;
        smLookups.RGB2YUV_UB[i] = (439*i)/1000;

        smLookups.RGB2YUV_VR[i] = (439*i)/1000;
        smLookups.RGB2YUV_VG[i] = (-368*i)/1000;
        smLookups.RGB2YUV_VB[i] = (-71*i)/1000;
    }

    //
    // Tables to convert YUV components to RGB components
    //
    smLookups.YUV2RGB_Y = new signed short[256];
    for ( int i = 0; i < 256; i++ )
    {
        int y = (i<16?16:i>235?235:i);

        smLookups.YUV2RGB_Y[i] = (1164*(y-16))/1000;
    }
    smLookups.YUV2RGB_RV = new signed short[256];
    smLookups.YUV2RGB_GU = new signed short[256];
    smLookups.YUV2RGB_GV = new signed short[256];
    smLookups.YUV2RGB_BU = new signed short[256];
    for ( int i = 0; i < 256; i++ )
    {
        int uv = (i<16?16:i>240?240:i);

        smLookups.YUV2RGB_RV[i] = (1596*(uv-128))/1000;
        smLookups.YUV2RGB_GU[i] = (-391*(uv-128))/1000;
        smLookups.YUV2RGB_GV[i] = (-813*(uv-128))/1000;
        smLookups.YUV2RGB_BU[i] = (2018*(uv-128))/1000;
    }
    initMutex.unlock();
}

// Copies and expands/compresses YUV planar data
/**
* @brief 
*
* @param pDst
* @param pSrc
* @param width
* @param height
* @param densityX
* @param densityY
* @param expand
*/
void Image::copyYUVP2YUVP( unsigned char *pDst, const unsigned char *pSrc, int width, int height, int densityX, int densityY, bool expand )
{
    if ( expand )
    {
        size_t srcPlaneSize = width*height;
        size_t srcUvPlaneWidth = width/densityX;
        size_t srcUvPlaneHeight = height/densityY;

        memcpy( pDst, pSrc, srcPlaneSize );
        pDst += srcPlaneSize;
        pSrc += srcPlaneSize;
        for ( int y = 0; y < srcUvPlaneHeight*2; y++ )
        {
            for ( int x = 0; x < srcUvPlaneWidth; x++ )
            {
                for ( int x2 = 0; x2 < densityX; x2++ )
                {
                    *pDst++ = *pSrc;
                }
                pSrc++;
            }
            for ( int y2 = 1; y2 < densityY; y2++ )
            {
                memcpy( pDst, pDst-width, width );
                pDst += width;
            }
        }
    }
    else
    {
        size_t srcPlaneSize = width*height;
        size_t dstUvPlaneWidth = width/densityX;
        size_t dstUvPlaneHeight = height/densityY;
        size_t density = densityX * densityY;

        memcpy( pDst, pSrc, srcPlaneSize );
        pDst += srcPlaneSize;
        pSrc += srcPlaneSize;
        unsigned short total = 0;
        for ( int y = 0; y < dstUvPlaneHeight*2; y++ )
        {
            for ( int x = 0; x < dstUvPlaneWidth; x++ )
            {
                total = 0;
                for ( int y2 = 0; y2 < densityY; y2++ )
                {
                    for ( int x2 = 0; x2 < densityX; x2++ )
                    {
                        total += *(pSrc+(y2*width)+x2);
                    }
                }
                *pDst++ = total/density;
                pSrc += densityX;
            }
            pSrc += (width*densityY);
        }
    }
}

/**
* @brief 
*
* @param pDst
* @param pSrc
* @param width
* @param height
*/
void Image::copyYUYV2RGB( unsigned char *pDst, const unsigned char *pSrc, int width, int height )
{
    size_t size = width*height*2;
    int y1,y2,u,v;
    for ( int i = 0; i < size; i += 4 )
    {
        y1 = *pSrc++;
        u = *pSrc++;
        y2 = *pSrc++;
        v = *pSrc++;

        *pDst++ = YUV2RGB_R(y1,u,v);
        *pDst++ = YUV2RGB_G(y1,u,v);
        *pDst++ = YUV2RGB_B(y1,u,v);
        *pDst++ = YUV2RGB_R(y2,u,v);
        *pDst++ = YUV2RGB_G(y2,u,v);
        *pDst++ = YUV2RGB_B(y2,u,v);
    }
}

/**
* @brief 
*
* @param pDst
* @param pSrc
* @param width
* @param height
*/
void Image::copyUYVY2RGB( unsigned char *pDst, const unsigned char *pSrc, int width, int height )
{
    size_t size = width*height*2;
    int y1,y2,u,v;
    for ( int i = 0; i < size; i += 4 )
    {
        u = *pSrc++;
        y1 = *pSrc++;
        v = *pSrc++;
        y2 = *pSrc++;

        *pDst++ = YUV2RGB_R(y1,u,v);
        *pDst++ = YUV2RGB_G(y1,u,v);
        *pDst++ = YUV2RGB_B(y1,u,v);
        *pDst++ = YUV2RGB_R(y2,u,v);
        *pDst++ = YUV2RGB_G(y2,u,v);
        *pDst++ = YUV2RGB_B(y2,u,v);
    }
}

/**
* @brief 
*
* @param pDst
* @param pSrc
* @param width
* @param height
*/
void Image::copyUYVY2YUVP( unsigned char *pDst, const unsigned char *pSrc, int width, int height )
{
    //Hexdump( 0, pSrc, 16 );
    size_t size = width*height*2;
    unsigned char *yDst = pDst;
    unsigned char *uDst = yDst+(width*height);
    unsigned char *vDst = uDst+(width*height);
    for ( int i = 0; i < size; i += 4 )
    {
        *uDst++ = *pSrc;
        *uDst++ = *pSrc++;
        *yDst++ = *pSrc++;
        *vDst++ = *pSrc;
        *vDst++ = *pSrc++;
        *yDst++ = *pSrc++;
    }
}

/**
* @brief 
*
* @param pDst
* @param pSrc
* @param width
* @param height
*/
void Image::copyYUYV2YUVP( unsigned char *pDst, const unsigned char *pSrc, int width, int height )
{
    size_t size = width*height*2;
    unsigned char *yDst = pDst;
    unsigned char *uDst = yDst+(width*height);
    unsigned char *vDst = uDst+(width*height);
    for ( int i = 0; i < size; i += 4 )
    {
        *yDst++ = *pSrc++;
        *uDst++ = *pSrc;
        *uDst++ = *pSrc++;
        *yDst++ = *pSrc++;
        *vDst++ = *pSrc;
        *vDst++ = *pSrc++;
    }
}

// Broken, do not use
// In this case the P means packed and not planar
/**
* @brief 
*
* @param pDst
* @param pSrc
* @param width
* @param height
* @param rBits
* @param gBits
* @param bBits
*/
void Image::copyRGBP2RGB( unsigned char *pDst, const unsigned char *pSrc, int width, int height, int rBits, int gBits, int bBits )
{
    int packSize = (((rBits+gBits+bBits)-1)/8)+1;
    uint8_t packBits = packSize*8;
    int size = width*height;
    if ( packSize == 1 )
    {
        uint8_t rShift = gBits+bBits;
        uint8_t rMask = (0xff>>(packBits-rBits))<<rShift;
        uint8_t gShift = bBits;;
        uint8_t gMask = (0xff>>(packBits-gBits))<<gShift;
        uint8_t bShift = 0;
        uint8_t bMask = (0xff>>(packBits-bBits))<<bShift;
        for ( int i = 0; i < size; i++ )
        {
            *pDst++ = (*pSrc&rMask)>>rShift;
            *pDst++ = (*pSrc&gMask)>>gShift;
            *pDst++ = (*pSrc&bMask); // pDst++ = (*src&bMask)>>bShift;
            pSrc++;
        }
    }
    else if ( packSize == 2 )
    {
        uint16_t *src = (uint16_t *)pSrc;
        uint8_t rShift = gBits+bBits;
        uint16_t rMask = (0xffff>>(packBits-rBits))<<rShift;
        uint8_t gShift = bBits;;
        uint16_t gMask = (0xffff>>(packBits-gBits))<<gShift;
        uint8_t bShift = 0;
        uint16_t bMask = (0xffff>>(packBits-bBits))<<bShift;
        for ( int i = 0; i < size; i++ )
        {
            *pDst++ = (*src&rMask)>>rShift;
            *pDst++ = (*src&gMask)>>gShift;
            *pDst++ = (*src&bMask); // pDst++ = (*src&bMask)>>bShift;
            src++;
        }
    }
    else
    {
        Panic( "Can't expand RGB packed formats using more than two bytes" );
    }
}

/**
* @brief 
*
* @param pDst
* @param pSrc
* @param width
* @param height
*/
void Image::copyBGR2RGB( unsigned char *pDst, const unsigned char *pSrc, int width, int height )
{
    int size = width*height*3;
    for ( int i = 0; i < size; i += 3 )
    {
        *pDst++ = *(pSrc+2);
        *pDst++ = *(pSrc+1);
        *pDst++ = *pSrc;
        pSrc += 3;
    }
}

/**
* @brief 
*
* @param palette
*
* @return 
*/
#ifdef HAVE_LINUX_VIDEODEV2_H
Image::Format Image::getFormatFromPalette( int palette )
{
    Format format = FMT_UNDEF;
    switch( palette )
    {
        case V4L2_PIX_FMT_BGR24 :
            format = FMT_RGB;
            break;
        case V4L2_PIX_FMT_GREY :
            format = FMT_GREY;
            break;
        case V4L2_PIX_FMT_YUV422P :
        case V4L2_PIX_FMT_YUV411P :
        case V4L2_PIX_FMT_YUV410 :
        case V4L2_PIX_FMT_YUV420 :
            format = FMT_YUVP;
            break;
        case V4L2_PIX_FMT_JPEG :
            format = FMT_YUVJP;
            break;
        case V4L2_PIX_FMT_YUYV :
        case V4L2_PIX_FMT_UYVY :
            format = FMT_YUV;
            break;
        default :
            Panic( "Can't find format for palette %d", palette );
            break;
    }
    return( format );
}
#endif

/**
* @brief 
*
* @param format
*
* @return 
*/
AVPixelFormat Image::getFfPixFormat( Format format )
{
    AVPixelFormat pixFormat = AV_PIX_FMT_NONE;
    switch( format )
    {
        case FMT_GREY :
            pixFormat = AV_PIX_FMT_GRAY8;
            break;
        case FMT_GREY16 :
            pixFormat = AV_PIX_FMT_GRAY16;
            break;
        case FMT_RGB :
            pixFormat = AV_PIX_FMT_RGB24;
            break;
        case FMT_RGB48 :
            pixFormat = AV_PIX_FMT_RGB48BE;
            break;
        case FMT_YUVP :
            pixFormat = AV_PIX_FMT_YUV444P;
            break;
        case FMT_YUVJP :
            pixFormat = AV_PIX_FMT_YUVJ444P;
            break;
        case FMT_UNDEF :
        default :
        {
            Panic( "Can't find swscale format for format %d", format );
            break;
        }
    }
    return( pixFormat );
}

/**
* @brief 
*
* @param pixelFormat
*
* @return 
*/
Image::Format Image::getFormatFromPixelFormat( AVPixelFormat pixelFormat )
{
    Format format = FMT_UNDEF;

    switch( pixelFormat )
    {
        case AV_PIX_FMT_GRAY8 :     ///<        Y        ,  8bpp
        {
            format = FMT_GREY;
            break;
        }
        case AV_PIX_FMT_GRAY16 :
        {
            format = FMT_GREY16;
            break;
        }
        case AV_PIX_FMT_RGB24 :     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
        case AV_PIX_FMT_BGR24 :     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
        case AV_PIX_FMT_RGB565:
        case AV_PIX_FMT_RGB555:
        {
            format = FMT_RGB;
            break;
        }
        case AV_PIX_FMT_YUYV422 :   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
        case AV_PIX_FMT_UYVY422 :   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
        case AV_PIX_FMT_YUV410P :   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
        case AV_PIX_FMT_YUV420P :   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
        case AV_PIX_FMT_YUV440P :   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
        case AV_PIX_FMT_YUV411P :   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
        case AV_PIX_FMT_YUV422P :   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
        case AV_PIX_FMT_YUV444P :   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
        {
            format = FMT_YUVP;
            break;
        }
        case AV_PIX_FMT_YUVJ420P :  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
        case AV_PIX_FMT_YUVJ440P :  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV440P and setting color_range
        case AV_PIX_FMT_YUVJ422P :  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV422P and setting color_range
        case AV_PIX_FMT_YUVJ444P :  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV444P and setting color_range
        {
            format = FMT_YUVJP;
            break;
        }
        case AV_PIX_FMT_RGB48:
        {
            format = FMT_RGB48;
            break;
        }
        default :
        {
            Panic( "Can't convert PixelFormat %d to image format", pixelFormat );
            break;
        }
    }
    return( format );
}

/**
* @brief 
*
* @param transparency
*
* @return 
*/
Image::BlendTablePtr Image::getBlendTable( int transparency )
{
    static Mutex blendMutex;

    blendMutex.lock();
    BlendTablePtr blendPtr = smBlendTables[transparency];
    if ( !blendPtr )
    {
        blendPtr = smBlendTables[transparency] = new BlendTable[1];
        //Info( "Generating blend table for transparency %d", transparency );
        int opacity = 100-transparency;
        //int round_up = 50/transparency;
        for ( int i = 0; i < 256; i++ )
        {
            for ( int j = 0; j < 256; j++ )
            {
                //(*blendPtr)[i][j] = (unsigned char)((((i + round_up) * opacity)+((j + round_up) * transparency))/100);
                (*blendPtr)[i][j] = (unsigned char)(((i * opacity)+(j * transparency))/100);
                //printf( "I:%d, J:%d, B:%d\n", i, j, (*blendPtr)[i][j] );
            }
        }
    }
    blendMutex.unlock();
    return( blendPtr );
}

unsigned char Image::red( int p ) const
{
    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_GREY16 :
            return( mBuffer[p*mPixelStep] );
        case FMT_RGB :
        case FMT_RGB48 :
            return( mBuffer[p*mPixelStep] );
        case FMT_YUV :
            return( YUV2RGB_R(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_YUVP :
            return( YUV2RGB_R(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+mPlaneSize],mBuffer[(p*mPixelStep)+(2*mPlaneSize)]) );
        case FMT_YUVJ :
            return( YUVJ2RGB_R(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_YUVJP :
            return( YUVJ2RGB_R(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+mPlaneSize],mBuffer[(p*mPixelStep)+(2*mPlaneSize)]) );
        case FMT_UNDEF :
            break;
    }
    return( 0 );
}

/**
* @brief 
*
* @param p
*
* @return 
*/
unsigned char Image::green( int p ) const
{
    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_GREY16 :
            return( mBuffer[p*mPixelStep] );
        case FMT_RGB :
            return( mBuffer[(p*mPixelStep)+1] );
        case FMT_RGB48 :
            return( mBuffer[(p*mPixelStep)+mPixelWidth] );
        case FMT_YUV :
            return( YUV2RGB_G(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_YUVP :
            return( YUV2RGB_G(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+mPlaneSize],mBuffer[(p*mPixelStep)+(2*mPlaneSize)]) );
        case FMT_YUVJ :
            return( YUVJ2RGB_G(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_YUVJP :
            return( YUVJ2RGB_G(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+mPlaneSize],mBuffer[(p*mPixelStep)+(2*mPlaneSize)]) );
        case FMT_UNDEF :
            break;
    }
    return( 0 );
}

/**
* @brief 
*
* @param p
*
* @return 
*/
unsigned char Image::blue( int p ) const
{
    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_GREY16 :
            return( mBuffer[p*mPixelStep] );
        case FMT_RGB :
            return( mBuffer[(p*mPixelStep)+2] );
        case FMT_RGB48 :
            return( mBuffer[(p*mPixelStep)+(2*mPixelWidth)] );
        case FMT_YUV :
            return( YUV2RGB_B(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_YUVP :
            return( YUV2RGB_B(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+mPlaneSize],mBuffer[(p*mPixelStep)+(2*mPlaneSize)]) );
        case FMT_YUVJ :
            return( YUVJ2RGB_B(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_YUVJP :
            return( YUVJ2RGB_B(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+mPlaneSize],mBuffer[(p*mPixelStep)+(2*mPlaneSize)]) );
        case FMT_UNDEF :
            break;
    }
    return( 0 );
}

/**
* @brief 
*
* @param x
* @param y
*
* @return 
*/
unsigned char Image::red( int x, int y ) const
{
    int offset = (x+(y*mWidth))*mPixelStep;
    return( Image::red( offset ) );
}

/**
* @brief 
*
* @param x
* @param y
*
* @return 
*/
unsigned char Image::green( int x, int y ) const
{
    int offset = (x+(y*mWidth))*mPixelStep;
    return( Image::green( offset ) );
}

/**
* @brief 
*
* @param x
* @param y
*
* @return 
*/
unsigned char Image::blue( int x, int y ) const
{
    int offset = (x+(y*mWidth))*mPixelStep;
    return( Image::blue( offset ) );
}

/**
* @brief 
*
* @param p
*
* @return 
*/
unsigned char Image::y( int p ) const
{
    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_GREY16 :
            return( mBuffer[p*mPixelStep] );
        case FMT_RGB :
            return(RGB2YUVJ_Y(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_RGB48 :
            return(RGB2YUVJ_Y(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+2],mBuffer[(p*mPixelStep)+4]) );
        case FMT_YUV :
            return( YUV2YUVJ_Y(mBuffer[p*mPixelStep]) );
        case FMT_YUVP :
            return( YUV2YUVJ_Y(mBuffer[p]) );
        case FMT_YUVJ :
            return( mBuffer[p*mPixelStep] );
        case FMT_YUVJP :
            return( mBuffer[p] );
        case FMT_UNDEF :
            break;
    }
    return( 0 );
}

/**
* @brief 
*
* @param p
*
* @return 
*/
unsigned char Image::u( int p ) const
{
    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_GREY16 :
            return( 128 );
        case FMT_RGB :
            return(RGB2YUVJ_U(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_RGB48 :
            return(RGB2YUVJ_U(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+2],mBuffer[(p*mPixelStep)+4]) );
        case FMT_YUV :
            return( YUV2YUVJ_U(mBuffer[(p*mPixelStep)+1]) );
        case FMT_YUVP :
            return( YUV2YUVJ_U(mBuffer[p+mPlaneSize]) );
        case FMT_YUVJ :
            return( mBuffer[(p*mPixelStep)+1] );
        case FMT_YUVJP :
            return( mBuffer[p+mPlaneSize] );
        case FMT_UNDEF :
            break;
    }
    return( 128 );
}

/**
* @brief 
*
* @param p
*
* @return 
*/
unsigned char Image::v( int p ) const
{
    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_GREY16 :
            return( 128 );
        case FMT_RGB :
            return(RGB2YUVJ_V(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+1],mBuffer[(p*mPixelStep)+2]) );
        case FMT_RGB48 :
            return(RGB2YUVJ_V(mBuffer[p*mPixelStep],mBuffer[(p*mPixelStep)+2],mBuffer[(p*mPixelStep)+4]) );
        case FMT_YUV :
            return( YUV2YUVJ_V(mBuffer[(p*mPixelStep)+2]) );
        case FMT_YUVP :
            return( YUV2YUVJ_V(mBuffer[p+(2*mPlaneSize)]) );
        case FMT_YUVJ :
            return( mBuffer[(p*mPixelStep)+2] );
        case FMT_YUVJP :
            return( mBuffer[p+(2*mPlaneSize)] );
        case FMT_UNDEF :
            break;
    }
    return( 128 );
}

/**
* @brief 
*
* @param x
* @param y
*
* @return 
*/
unsigned char Image::y( int x, int y ) const
{
    int offset = (x+(y*mWidth))*mPixelStep;
    return( Image::y( offset ) );
}

/**
* @brief 
*
* @param x
* @param y
*
* @return 
*/
unsigned char Image::u( int x, int y ) const
{
    int offset = (x+(y*mWidth))*mPixelStep;
    return( Image::u( offset ) );
}

/**
* @brief 
*
* @param x
* @param y
*
* @return 
*/
unsigned char Image::v( int x, int y ) const
{
    int offset = (x+(y*mWidth))*mPixelStep;
    return( Image::v( offset ) );
}

/**
* @brief 
*
* @param lines
* @param cols
*/
void Image::dump( int lines, int cols ) const
{
    Debug( 1,"DUMP-F%d, %dx%d", mFormat, mWidth, mHeight );
    if ( lines > 0 )
    {
        if ( lines > mHeight )
            lines = mHeight;
        if ( cols <= 0 || cols > mWidth )
            cols = mWidth;
        if ( mPlanes == 1 )
        {
            Debug( 1,"DUMP-%d channels", mChannels );
            for ( int y = 0; y < lines; y++ )
            {
                Hexdump( 0, mBuffer.data()+(mStride*y), cols*mChannels*mPixelWidth );
            }
        }
        else
        {
            unsigned char *yPtr = mBuffer.data();
            Debug( 1,"DUMP-Y-Channel" );
            for ( int y = 0; y < lines; y++ )
            {
                Hexdump( 0, yPtr+(mStride*y), cols*mPixelWidth );
            }
            if ( mChannels > 1 )
            {
                unsigned char *uPtr = yPtr+mPlaneSize;
                Debug( 1,"DUMP-U-Channel" );
                for ( int y = 0; y < lines; y++ )
                {
                    Hexdump( 0, uPtr+(mStride*y), cols*mPixelWidth );
                }
                unsigned char *vPtr = uPtr+mPlaneSize;
                Info( "DUMP-V-Channel" );
                for ( int y = 0; y < lines; y++ )
                {
                    Hexdump( 0, vPtr+(mStride*y), cols*mPixelWidth );
                }
            }
        }
    }
}

/**
* @brief 
*/
void Image::clear()
{
    mColourSpace = CS_UNDEF;
    mFormat = FMT_UNDEF;
    mWidth = 0;
    mHeight = 0;
    mPixels = 0;
    mChannels = 0;
    mPlanes = 0;
    mColourDepth = 0;
    mPixelWidth = 0;
    mPixelStep = 0;
    mStride = 0;
    mBuffer.clear();
    mPlaneSize = 0;
    mSize = 0;
    mText.clear();
    mMask = 0;
}

/**
* @brief 
*
* @param image
*/
void Image::copy( const Image &image )
{
    mColourSpace = image.mColourSpace;
    mFormat = image.mFormat;
    mWidth = image.mWidth;
    mHeight = image.mHeight;
    mPixels = image.mPixels;
    mChannels = image.mChannels;
    mPlanes = image.mPlanes;
    mColourDepth = image.mColourDepth;
    mPixelWidth = image.mPixelWidth;
    mPixelStep = image.mPixelStep;
    mStride = image.mStride;
    mPlaneSize = image.mPlaneSize;
    mSize = image.mSize;
    mBuffer = image.mBuffer;
    mText = image.mText;
    mMask = image.mMask;
}

/**
* @brief 
*
* @param image
*/
void Image::convert( const Image &image )
{
    if ( !(mWidth == image.mWidth && mHeight == image.mHeight) )
        Panic( "Attempt to convert different sized images, expected %dx%d, got %dx%d", mWidth, mHeight, image.mWidth, image.mHeight );
    if ( mFormat == FMT_UNDEF || image.mFormat == FMT_UNDEF )
        Panic( "Attempt to convert to or from image with undefined format" );
    if ( mFormat == image.mFormat )
    {
        // No conversion necessary
        mBuffer = image.mBuffer;
        return;
    }

    unsigned char *pDst = mBuffer.data();
    unsigned char *pSrc = image.mBuffer.data();

    mBuffer.erase();

    switch( mFormat )
    {
        case FMT_GREY :
        {
            switch( image.mFormat )
            {
                case FMT_GREY16 :
                case FMT_YUVJ :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = *pSrc;
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = RGB2YUVJ_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = RGB2YUVJ_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = YUV2YUVJ_Y(*pSrc);
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = YUV2YUVJ_Y(*pSrc++);
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = *pSrc++;
                    }
                    return;
                }
                case FMT_UNDEF :
                case FMT_GREY :
                    break;
            }
            break;
        }
        case FMT_GREY16 :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_YUVJP :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = *pSrc++;
                        //*(pDst+1) = 0;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = RGB2YUVJ_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                        //*(pDst+1) = 0;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = RGB2YUVJ_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                        //*(pDst+1) = 0;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUV2YUVJ_Y(*pSrc);
                        //*(pDst+1) = 0;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = *pSrc;
                        //*(pDst+1) = 0;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUV2YUVJ_Y(*pSrc++);
                        //*(pDst+1) = 0;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                case FMT_GREY16 :
                    break;
            }
            break;
        }
        case FMT_RGB :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = *(pDst+1) = *(pDst+2) = *pSrc;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    // Only copy over the top bytes, for rgb 48
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = *pSrc;
                        *(pDst+1) = *(pSrc+2);
                        *(pDst+2) = *(pSrc+4);
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUV2RGB_R(*(pSrc),*(pSrc+1),*(pSrc+2));
                        *(pDst+1) = YUV2RGB_G(*(pSrc),*(pSrc+1),*(pSrc+2));
                        *(pDst+2) = YUV2RGB_B(*(pSrc),*(pSrc+1),*(pSrc+2));
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUVJ2RGB_R(*(pSrc),*(pSrc+1),*(pSrc+2));
                        *(pDst+1) = YUVJ2RGB_G(*(pSrc),*(pSrc+1),*(pSrc+2));
                        *(pDst+2) = YUVJ2RGB_B(*(pSrc),*(pSrc+1),*(pSrc+2));
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mPlaneSize);
                    unsigned char *pSrcV = pSrcU+(image.mPlaneSize);
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUV2RGB_R(*pSrcY,*pSrcU,*pSrcV);
                        *(pDst+1) = YUV2RGB_G(*pSrcY,*pSrcU,*pSrcV);
                        *(pDst+2) = YUV2RGB_B(*pSrcY,*pSrcU,*pSrcV);
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mPlaneSize);
                    unsigned char *pSrcV = pSrcU+(image.mPlaneSize);
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUVJ2RGB_R(*pSrcY,*pSrcU,*pSrcV);
                        *(pDst+1) = YUVJ2RGB_G(*pSrcY,*pSrcU,*pSrcV);
                        *(pDst+2) = YUVJ2RGB_B(*pSrcY,*pSrcU,*pSrcV);
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                case FMT_RGB :
                    break;
            }
            break;
        }
        case FMT_RGB48 :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = *(pDst+2) = *(pDst+4) = *pSrc;
                        //*(pDst+1) = *(pDst+3) = *(pDst+5) = *pSrc;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    // Only copy over the top bytes, for rgb 48
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = *pSrc;
                        *(pDst+2) = *(pSrc+1);
                        *(pDst+4) = *(pSrc+2);
                        //*(pDst+1) = *(pDst+3) = *(pDst+5) = *pSrc;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUV2RGB_R(*(pSrc),*(pSrc+1),*(pSrc+2));
                        *(pDst+2) = YUV2RGB_G(*(pSrc),*(pSrc+1),*(pSrc+2));
                        *(pDst+4) = YUV2RGB_B(*(pSrc),*(pSrc+1),*(pSrc+2));
                        //*(pDst+1) = *(pDst+3) = *(pDst+5) = *pSrc;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUVJ2RGB_R(*(pSrc),*(pSrc+1),*(pSrc+2));
                        *(pDst+2) = YUVJ2RGB_G(*(pSrc),*(pSrc+1),*(pSrc+2));
                        *(pDst+4) = YUVJ2RGB_B(*(pSrc),*(pSrc+1),*(pSrc+2));
                        //*(pDst+1) = *(pDst+3) = *(pDst+5) = *pSrc;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUV2RGB_R(*pSrcY,*pSrcU,*pSrcV);
                        *(pDst+2) = YUV2RGB_G(*pSrcY,*pSrcU,*pSrcV);
                        *(pDst+4) = YUV2RGB_B(*pSrcY,*pSrcU,*pSrcV);
                        //*(pDst+1) = *(pDst+3) = *(pDst+5) = *pSrc;
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst = YUVJ2RGB_R(*pSrcY,*pSrcU,*pSrcV);
                        *(pDst+2) = YUVJ2RGB_G(*pSrcY,*pSrcU,*pSrcV);
                        *(pDst+4) = YUVJ2RGB_B(*pSrcY,*pSrcU,*pSrcV);
                        //*(pDst+1) = *(pDst+3) = *(pDst+5) = *pSrc;
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                case FMT_RGB48 :
                    break;
            }
            break;
        }
        case FMT_YUV :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = YUVJ2YUV_Y(*pSrc);
                        *pDst++ = -128;
                        *pDst++ = -128;
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = RGB2YUV_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                        *pDst++ = RGB2YUV_U(*pSrc,*(pSrc+1),*(pSrc+2));
                        *pDst++ = RGB2YUV_V(*pSrc,*(pSrc+1),*(pSrc+2));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = RGB2YUV_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                        *pDst++ = RGB2YUV_U(*pSrc,*(pSrc+2),*(pSrc+4));
                        *pDst++ = RGB2YUV_V(*pSrc,*(pSrc+2),*(pSrc+4));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = YUVJ2YUV_Y(*pSrc++);
                        *pDst++ = YUVJ2YUV_U(*pSrc++);
                        *pDst++ = YUVJ2YUV_V(*pSrc++);
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = *pSrcY++;
                        *pDst++ = *pSrcU++;
                        *pDst++ = *pSrcV++;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = YUVJ2YUV_Y(*pSrcY++);
                        *pDst++ = YUVJ2YUV_U(*pSrcU++);
                        *pDst++ = YUVJ2YUV_V(*pSrcV++);
                    }
                    return;
                }
                case FMT_UNDEF :
                case FMT_YUV :
                    break;
            }
            break;
        }
        case FMT_YUVJ :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = *pSrc;
                        *pDst++ = -128;
                        *pDst++ = -128;
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = RGB2YUVJ_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                        *pDst++ = RGB2YUVJ_U(*pSrc,*(pSrc+1),*(pSrc+2));
                        *pDst++ = RGB2YUVJ_V(*pSrc,*(pSrc+1),*(pSrc+2));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = RGB2YUVJ_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                        *pDst++ = RGB2YUVJ_U(*pSrc,*(pSrc+2),*(pSrc+4));
                        *pDst++ = RGB2YUVJ_V(*pSrc,*(pSrc+2),*(pSrc+4));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = YUV2YUVJ_Y(*pSrc++);
                        *pDst++ = YUV2YUVJ_U(*pSrc++);
                        *pDst++ = YUV2YUVJ_V(*pSrc++);
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = YUV2YUVJ_Y(*pSrcY++);
                        *pDst++ = YUV2YUVJ_U(*pSrcU++);
                        *pDst++ = YUV2YUVJ_V(*pSrcV++);
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        *pDst++ = *pSrcY++;
                        *pDst++ = *pSrcU++;
                        *pDst++ = *pSrcV++;
                    }
                    return;
                }
                case FMT_UNDEF :
                case FMT_YUVJ :
                    break;
            }
            break;
        }
        case FMT_YUVP :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                {
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDst++ = YUVJ2YUV_Y(*pSrc++);
                    }
                    memset( pDst, -128, 2*mPlaneSize );
                    return;
                }
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDst++ = YUVJ2YUV_Y(*pSrc);
                        pSrc += image.mPixelStep;
                    }
                    memset( pDst, -128, 2*mPlaneSize );
                    return;
                }
                case FMT_RGB :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDstY++ = RGB2YUV_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                        *pDstU++ = RGB2YUV_U(*pSrc,*(pSrc+1),*(pSrc+2));
                        *pDstV++ = RGB2YUV_V(*pSrc,*(pSrc+1),*(pSrc+2));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDstY++ = RGB2YUV_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                        *pDstU++ = RGB2YUV_U(*pSrc,*(pSrc+2),*(pSrc+4));
                        *pDstV++ = RGB2YUV_V(*pSrc,*(pSrc+2),*(pSrc+4));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDstY++ = *pSrc++;
                        *pDstU++ = *pSrc++;
                        *pDstV++ = *pSrc++;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDstY++ = YUVJ2YUV_Y(*pSrc++);
                        *pDstU++ = YUVJ2YUV_U(*pSrc++);
                        *pDstV++ = YUVJ2YUV_V(*pSrc++);
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrcV < image.mBuffer.tail() )
                    {
                        *pDstY++ = YUVJ2YUV_Y(*pSrcY++);
                        *pDstU++ = YUVJ2YUV_U(*pSrcU++);
                        *pDstV++ = YUVJ2YUV_V(*pSrcV++);
                    }
                    return;
                }
                case FMT_UNDEF :
                case FMT_YUVP :
                    break;
            }
            break;
        }
        case FMT_YUVJP :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                {
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDst++ = *pSrc++;
                    }
                    memset( pDst, -128, 2*mPlaneSize );
                    return;
                }
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDst++ = *pSrc;
                        pSrc += image.mPixelStep;
                    }
                    memset( pDst, -128, 2*mPlaneSize );
                    return;
                }
                case FMT_RGB :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDstY++ = RGB2YUVJ_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                        *pDstU++ = RGB2YUVJ_U(*pSrc,*(pSrc+1),*(pSrc+2));
                        *pDstV++ = RGB2YUVJ_V(*pSrc,*(pSrc+1),*(pSrc+2));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDstY++ = RGB2YUVJ_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                        *pDstU++ = RGB2YUVJ_U(*pSrc,*(pSrc+2),*(pSrc+4));
                        *pDstV++ = RGB2YUVJ_V(*pSrc,*(pSrc+2),*(pSrc+4));
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDstY++ = YUV2YUVJ_Y(*pSrc++);
                        *pDstU++ = YUV2YUVJ_U(*pSrc++);
                        *pDstV++ = YUV2YUVJ_V(*pSrc++);
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        *pDstY++ = *pSrc++;
                        *pDstU++ = *pSrc++;
                        *pDstV++ = *pSrc++;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrcV < image.mBuffer.tail() )
                    {
                        *pDstY++ = YUV2YUVJ_Y(*pSrcY++);
                        *pDstU++ = YUV2YUVJ_U(*pSrcU++);
                        *pDstV++ = YUV2YUVJ_V(*pSrcV++);
                    }
                    return;
                }
                case FMT_UNDEF :
                case FMT_YUVJP :
                    break;
            }
            break;
        }
        case FMT_UNDEF :
            break;
    }
    Panic( "Unsupported conversion to format %d from format %d", mFormat, image.mFormat );
}

/**
* @brief 
*/
Image::Image()
{
    if ( !smInitialised )
        initialise();
    clear();
}

/**
* @brief 
*
* @param filename
*/
Image::Image( const char *filename )
{
    if ( !smInitialised )
        initialise();
    clear();
    readJpeg( filename );
}

/**
* @brief 
*
* @param format
* @param width
* @param height
* @param data
* @param adoptData
*/
Image::Image( Format format, int width, int height, unsigned char *data, bool adoptData )
{
    if ( !smInitialised )
        initialise();
    clear();
    assign( format, width, height, data, adoptData );
}

/**
* @brief 
*
* @param v4lPalette
* @param width
* @param height
*/
#ifdef HAVE_LINUX_VIDEODEV2_H
size_t Image::calcBufferSize( int v4lPalette, int width, int height )
{
    size_t pixels = width*height;
    switch( v4lPalette )
    {
        case V4L2_PIX_FMT_YUV420 :
        case V4L2_PIX_FMT_YUV410 :
        case V4L2_PIX_FMT_YUV422P :
        case V4L2_PIX_FMT_YUV411P :
        case V4L2_PIX_FMT_YUYV :
        case V4L2_PIX_FMT_UYVY :
        {
            // Converts to YUV expanded planar format
            return( pixels*3 );
        }
        case V4L2_PIX_FMT_RGB555 :
        case V4L2_PIX_FMT_RGB565 :
        case V4L2_PIX_FMT_BGR24 :
        case V4L2_PIX_FMT_RGB24 :
        {
            // Converts to RGB format
            return( pixels*3 );
        }
        case V4L2_PIX_FMT_GREY :
        {
            // Converts to greyscale format
            return( pixels);
        }
    }
    Panic( "No conversion for unexpected V4L2 palette %08x", v4lPalette );
    return( 0 );
}
#endif

/**
* @brief 
*
* @param v4lPalette
* @param width
* @param height
* @param data
*/
#ifdef HAVE_LINUX_VIDEODEV2_H
Image::Image( int v4lPalette, int width, int height, unsigned char *data )
{
    if ( !smInitialised )
        initialise();
    clear();

    Format format = FMT_UNDEF;

    size_t bufferSize = calcBufferSize( v4lPalette, width, height );
    uint8_t *tempData = new uint8_t [bufferSize];
    uint8_t *imageData = data;
    switch( v4lPalette )
    {
        case V4L2_PIX_FMT_YUV420 :
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 2, 2 );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_YUV410 :
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 4, 4 );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_YUV422P :
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 2, 1 );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_YUV411P :
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 4, 1 );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_YUYV :
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUYV2YUVP( tempData, data, width, height );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_UYVY :
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyUYVY2YUVP( tempData, data, width, height );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_RGB555 :
        {
            format = FMT_RGB;
            if ( !data ) break;
            copyRGBP2RGB( tempData, data, width, height, 5, 5, 5 );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_RGB565 :
        {
            format = FMT_RGB;
            if ( !data ) break;
            copyRGBP2RGB( tempData, data, width, height, 5, 6, 5 );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_BGR24 :
        {
            format = FMT_RGB;
            if ( !data ) break;
            copyBGR2RGB( tempData, data, width, height );
            imageData = tempData;
            break;
        }
        case V4L2_PIX_FMT_RGB24 :
        {
            // Nothing to do
            format = FMT_RGB;
            break;
        }
        case V4L2_PIX_FMT_GREY :
        {
            // Ok 'as is'
            format = FMT_GREY;
            break;
        }
#if 0
        case V4L2_PIX_FMT_JPEG :
        {
            // Ok 'as is'
            format = FMT_YUVJP;
            break;
        }
#endif
        default :
        {
            Panic( "Can't convert V4L2 palette %08x to image format", v4lPalette );
            break;
        }
    }
    assign( format, width, height, imageData );
    delete[] tempData;
}
#endif

/**
* @brief 
*
* @param pixFormat
* @param width
* @param height
*/
size_t Image::calcBufferSize( AVPixelFormat pixFormat, int width, int height )
{
    size_t pixels = width*height;
    switch( pixFormat )
    {
        case AV_PIX_FMT_YUV420P :   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
        case AV_PIX_FMT_YUYV422 :   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
        case AV_PIX_FMT_UYVY422 :   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
        case AV_PIX_FMT_YUV422P :   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
        case AV_PIX_FMT_YUV444P :   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
        case AV_PIX_FMT_YUV410P :   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
        case AV_PIX_FMT_YUV411P :   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
        case AV_PIX_FMT_YUVJ420P :  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
        case AV_PIX_FMT_YUVJ422P :  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV422P and setting color_range
        case AV_PIX_FMT_YUVJ444P :  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV444P and setting color_range
        case AV_PIX_FMT_YUV440P :   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
        case AV_PIX_FMT_YUVJ440P :  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV440P and setting color_range
        {
            // Converts to YUV expanded planar format
            return( pixels*3 );
        }
        case AV_PIX_FMT_RGB24 :     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
        case AV_PIX_FMT_BGR24 :     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
        case AV_PIX_FMT_RGB565:
        case AV_PIX_FMT_RGB555:
        {
            // Converts to RGB format
            return( pixels*3 );
        }
        case AV_PIX_FMT_RGB48:
        {
            // Converts to RGB 48 bits format
            return( pixels*6 );
        }
        case AV_PIX_FMT_GRAY8 :     ///<        Y        ,  8bpp
        {
            // Converts to greyscale format
            return( pixels );
        }
        case AV_PIX_FMT_GRAY16 :
        {
            // Converts to greyscale 16 bits format
            return( pixels*2 );
        }
    }
    Panic( "No conversion for unexpected AVPixelFormat palette %d", pixFormat );
    return( 0 );
}

/**
* @brief 
*
* @param pixFormat
* @param width
* @param height
* @param data
*/
Image::Image( AVPixelFormat pixFormat, int width, int height, unsigned char *data )
{
    if ( !smInitialised )
        initialise();
    clear();

    Format format = FMT_UNDEF;

    size_t bufferSize = calcBufferSize( pixFormat, width, height );
    uint8_t *tempData = new uint8_t[bufferSize];
    uint8_t *imageData = data;
    switch( pixFormat )
    {
        case AV_PIX_FMT_YUV420P :   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 2, 2 );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_YUYV422 :   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUYV2YUVP( tempData, data, width, height );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_UYVY422 :   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyUYVY2RGB( tempData, data, width, height );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_RGB24 :     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
        {
            // Nothing to do
            format = FMT_RGB;
            break;
        }
        case AV_PIX_FMT_BGR24 :     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
        {
            format = FMT_RGB;
            if ( !data ) break;
            copyBGR2RGB( tempData, data, width, height );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_YUV422P :   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 2, 1 );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_YUV444P :   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
        {
            // XXX - Not convinced about this one
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 1, 1 );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_YUV410P :   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 4, 4 );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_YUV411P :   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
        {
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 4, 1 );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_GRAY8 :     ///<        Y        ,  8bpp
        {
            // Ok 'as is'
            format = FMT_GREY;
            break;
        }
        case AV_PIX_FMT_YUVJ420P :  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
        {
            format = FMT_YUVJP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 2, 2 );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_YUVJ422P :  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV422P and setting color_range
        {
            format = FMT_YUVJP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 2, 1 );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_YUVJ444P :  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV444P and setting color_range
        {
            format = FMT_YUVJP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 1, 1 );
            imageData = tempData;
            break;
        }
        //case AV_PIX_FMT_GRAY16BE :  ///<        Y        , 16bpp, big-endian
        //case AV_PIX_FMT_GRAY16LE :  ///<        Y        , 16bpp, little-endian
        case AV_PIX_FMT_GRAY16 :
        {
            // Ok 'as is'
            format = FMT_GREY16;
            break;
        }
        case AV_PIX_FMT_YUV440P :   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
        {
            // XXX - Not convinced about this one
            format = FMT_YUVP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 1, 2 );
            imageData = tempData;
            break;
        }
        case AV_PIX_FMT_YUVJ440P :  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV440P and setting color_range
        {
            // XXX - Not convinced about this one
            format = FMT_YUVJP;
            if ( !data ) break;
            copyYUVP2YUVP( tempData, data, width, height, 1, 2 );
            imageData = tempData;
            break;
        }
        //case AV_PIX_FMT_RGB48BE :   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as big-endian
        //case AV_PIX_FMT_RGB48LE :   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as little-endian
        case AV_PIX_FMT_RGB48:
        {
            // Ok 'as is'
            format = FMT_RGB48;
            break;
        }
        //case AV_PIX_FMT_RGB565BE :  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), big-endian
        //case AV_PIX_FMT_RGB565LE :  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), little-endian
        case AV_PIX_FMT_RGB565:
        {
            format = FMT_RGB;
            if ( !data ) break;
            copyRGBP2RGB( tempData, data, width, height, 5, 6, 5 );
            imageData = tempData;
            break;
        }
        //case AV_PIX_FMT_RGB555BE :  ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian, most significant bit to 0
        //case AV_PIX_FMT_RGB555LE :  ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian, most significant bit to 0
        case AV_PIX_FMT_RGB555:
        {
            format = FMT_RGB;
            if ( !data ) break;
            copyRGBP2RGB( tempData, data, width, height, 5, 5, 5 );
            imageData = tempData;
            break;
        }
        default :
        {
            Panic( "Can't convert AVPixelFormat %d to image format", pixFormat );
            break;
        }
    }
    assign( format, width, height, imageData );
    delete[] tempData;
}

/**
* @brief 
*
* @param image
*/
Image::Image( const Image &image )
{
    if ( !smInitialised )
        initialise();
    copy( image );
}

/**
* @brief 
*
* @param format
* @param image
*/
Image::Image( Format format, const Image &image )
{
    if ( !smInitialised )
        initialise();
    clear();
    assign( format, image.mWidth, image.mHeight );
    convert( image );
}

Image::~Image()
{
}

/**
* @brief 
*
* @param format
* @param width
* @param height
* @param data
* @param adoptData
*/
void Image::assign( Format format, int width, int height, unsigned char *data, bool adoptData )
{
    if ( mBuffer.empty() || format != mFormat || width != mWidth || height != mHeight )
    {
        mFormat = format;
        mWidth = width;
        mHeight = height;
        mPixels = mWidth*mHeight;

        switch( mFormat )
        {
            case FMT_GREY :
            {
                mColourSpace = CS_GREY;
                mChannels = 1;
                mPlanes = 1;
                mColourDepth = 8;
                break;
            }
            case FMT_GREY16 :
            {
                mColourSpace = CS_GREY;
                mChannels = 1;
                mPlanes = 1;
                mColourDepth = 16;
                break;
            }
            /*
            case FMT_HSV :
            {
                mColourSpace = CS_HSV;
                mChannels = 3;
                mPlanes = 1;
                mColourDepth = 8;
                break;
            }
            */
            case FMT_RGB :
            {
                mColourSpace = CS_RGB;
                mChannels = 3;
                mPlanes = 1;
                mColourDepth = 8;
                break;
            }
            case FMT_RGB48 :
            {
                mColourSpace = CS_RGB;
                mChannels = 3;
                mPlanes = 1;
                mColourDepth = 16;
                break;
            }
            case FMT_YUV :
            {
                mColourSpace = CS_YUV;
                mChannels = 3;
                mPlanes = 1;
                mColourDepth = 8;
                break;
            }
            case FMT_YUVJ :
            {
                mColourSpace = CS_YUVJ;
                mChannels = 3;
                mPlanes = 1;
                mColourDepth = 8;
                break;
            }
            case FMT_YUVP :
            {
                mColourSpace = CS_YUV;
                mChannels = 1;
                mPlanes = 3;
                mColourDepth = 8;
                break;
            }
            case FMT_YUVJP :
            {
                mColourSpace = CS_YUVJ;
                mChannels = 1;
                mPlanes = 3;
                mColourDepth = 8;
                break;
            }
            default :
            {
                throw Exception( stringtf( "Unexpected format %d found when creating image", mFormat ) );
                break;
            }
        }

        mPixelWidth = ((mColourDepth-1)/8)+1;
        mPixelStep = mChannels * mPixelWidth;
        mStride = mPixelStep * mWidth;
        mPlaneSize = mStride * mHeight;
        mSize = mPlaneSize * mPlanes;
    }
    if ( data )
        if ( adoptData )
            mBuffer.adopt( data, mSize );
        else
            mBuffer.assign( data, mSize );
    else
        mBuffer.size( mSize );
}

/**
* @brief 
*
* @param image
*/
void Image::assign( const Image &image )
{
    assign( image.mFormat, image.mWidth, image.mHeight, image.mBuffer.size()?image.mBuffer.data():0 );
}

/**
* @brief 
*
* @param colour
* @param mask
*
* @return 
*/
Image *Image::highlightEdges( Rgb colour, const Mask *mask ) const
{
    if ( mChannels != 1 )
        Panic( "Attempt to highlight image edges on non-greyscale image" );

    Image *hiImage = new Image( FMT_RGB, mWidth, mHeight );

    if ( !mask )
        mask = new BoxMask( Box( mWidth, mHeight ) );
    const Image *maskImage = mask->image();
    const ByteBuffer &maskBuffer = mask->image()->buffer();

    int maskLoY = mask->loY();
    int maskHiY = mask->hiY();
    int maskLoX = mask->loX();
    //int maskHiX = mask->hiX();

    for ( int y = maskLoY; y <= maskHiY; y++ )
    {
        int maskLineLoX = mask->loX(y-maskLoY);
        int maskLineHiX = mask->hiX(y-maskLoY);
        const unsigned char *pRef = mBuffer.data() + (mPixelStep*((y*mWidth)+maskLineLoX));
        const unsigned char *pMask = maskBuffer.data() + (maskLineLoX-maskLoX) + ((y-maskLoY)*maskImage->mWidth);
        unsigned char *pHi = hiImage->buffer().data() + (hiImage->mPixelStep*((y*mWidth)+maskLineLoX));
        for ( int x = maskLineLoX; x <= maskLineHiX; x++, pRef += mPixelStep, pMask++, pHi += hiImage->mPixelStep )
        {
            bool edge = false;
            if ( *pMask && *pRef )
            {
                if ( !edge && ((x == maskLineLoX) || (x > maskLineLoX && (!*(pMask-1) || !*(pRef-mPixelStep)))) ) edge = true;
                if ( !edge && ((x == maskLineHiX) || (x < maskLineHiX && (!*(pMask+1) || !*(pRef+mPixelStep)))) ) edge = true;
                if ( !edge && ((y == maskLoY) || (y > maskLoY && (!*(pMask-maskImage->mStride) || !*(pRef-mStride)))) ) edge = true;
                if ( !edge && ((y == maskHiY) || (y < maskHiY && (!*(pMask+maskImage->mStride) || !*(pRef+mStride)))) ) edge = true;
            }
            if ( edge )
            {
                RED(pHi) = RGB_RED_VAL(colour);
                GREEN(pHi) = RGB_GREEN_VAL(colour);
                BLUE(pHi) = RGB_BLUE_VAL(colour);
            }
        }
    }
    return( hiImage );
}

// Assumes raw files is basically byte dump of buffer so no format checking
/**
* @brief 
*
* @param filename
*
* @return 
*/
bool Image::readRaw( const char *filename )
{
    FILE *infile;
    if ( (infile = fopen( filename, "rb" )) == NULL )
    {
        Error( "Can't open %s: %s", filename, strerror(errno) );
        return( false );
    }

    struct stat statbuf;
    if ( fstat( fileno(infile), &statbuf ) < 0 )
    {
        Error( "Can't fstat %s: %s", filename, strerror(errno) );
        return( false );
    }

    if ( statbuf.st_size != mSize )
    {
        Error( "Raw file size mismatch, expected %d bytes, found %ld", mSize, statbuf.st_size );
        return( false );
    }

    mBuffer.size( mSize );
    if ( fread( mBuffer.data(), mSize, 1, infile ) < 1 )
    {
        Fatal( "Unable to read from '%s': %s", filename, strerror(errno) );
        return( false );
    }

    fclose( infile );

    return( true );
}

// Assumes raw files is basically byte dump of buffer so no format checking
/**
* @brief 
*
* @param filename
*
* @return 
*/
bool Image::writeRaw( const char *filename ) const
{
    FILE *outfile;
    if ( (outfile = fopen( filename, "wb" )) == NULL )
    {
        Error( "Can't open %s: %s", filename, strerror(errno) );
        return( false );
    }

    if ( fwrite( mBuffer.data(), mSize, 1, outfile ) != 1 )
    {
        Error( "Unable to write to '%s': %s", filename, strerror(errno) );
        return( false );
    }

    fclose( outfile );

    return( true );
}

/**
* @brief 
*
* @return 
*/
jpeg_decompress_struct *Image::jpegDcInfo()
{
    struct jpeg_decompress_struct *cinfo = smJpegDeCinfo;
    if ( !cinfo )
    {
        smJpegDeError.pub.error_exit = local_jpeg_error_exit;
        smJpegDeError.pub.emit_message = local_jpeg_emit_message;
        cinfo = smJpegDeCinfo = new jpeg_decompress_struct;
        memset( cinfo, 0, sizeof(*cinfo) );
        cinfo->err = jpeg_std_error( &smJpegDeError.pub );
        jpeg_create_decompress( cinfo );
    }
    return( cinfo );
}

/**
* @brief 
*
* @param quality
*
* @return 
*/
jpeg_compress_struct *Image::jpegCcInfo( int quality )
{
    struct jpeg_compress_struct *cinfo = smJpegCoCinfo[quality];
    if ( !cinfo )
    {
        smJpegCoError.pub.error_exit = local_jpeg_error_exit;
        smJpegCoError.pub.emit_message = local_jpeg_emit_message;
        cinfo = smJpegCoCinfo[quality] = new jpeg_compress_struct;
        memset( cinfo, 0, sizeof(*cinfo) );
        cinfo->err = jpeg_std_error( &smJpegCoError.pub );
        jpeg_create_compress( cinfo );
    }
    return( cinfo );
}

/**
* @brief 
*
* @param filename
*
* @return 
*/
bool Image::readJpeg( const std::string &filename )
{
    ScopedMutex mutex( smJpegDeMutex );

    struct jpeg_decompress_struct *cinfo = jpegDcInfo();

    FILE *infile;
    if ( (infile = fopen( filename.c_str(), "rb" )) == NULL )
    {
        Error( "Can't open %s: %s", filename.c_str(), strerror(errno) );
        return( false );
    }

    if ( setjmp( smJpegDeError.setjmp_buffer ) )
    {
        jpeg_abort_decompress( cinfo );
        fclose( infile );
        return( false );
    }

    jpeg_stdio_src( cinfo, infile );

    jpeg_read_header( cinfo, TRUE );

    J_COLOR_SPACE outColourSpace = JCS_UNKNOWN;
    switch( mFormat )
    {
        case FMT_GREY :
            // Jpeglib can't do RGB->Greyscale on input
            if ( cinfo->jpeg_color_space != JCS_RGB )
                outColourSpace = JCS_GRAYSCALE;
            break;
        case FMT_RGB :
            outColourSpace = JCS_RGB;
            break;
        default :
            break;
    }

    Format newFormat = mFormat;
    if ( outColourSpace != JCS_UNKNOWN )
        cinfo->out_color_space = outColourSpace;
    else
    {
        switch( cinfo->out_color_space )
        {
            case JCS_GRAYSCALE :
            {
                newFormat = FMT_GREY;
                break;
            }
            case JCS_RGB :
            {
                //cinfo->out_color_space = JCS_YCbCr;
                //newFormat = FMT_YUVJ;
                newFormat = FMT_RGB;
                break;
            }
            case JCS_YCbCr :
            {
                newFormat = FMT_YUVJ;
                break;
            }
            case JCS_UNKNOWN :
            case JCS_CMYK :
            case JCS_YCCK :
            default :
            {
                Error( "Unsupported colourspace %d found when reading JPEG file %s", cinfo->jpeg_color_space, filename.c_str() );
                jpeg_abort_decompress( cinfo );
                fclose( infile );
                return( false );
            }
        }
    }

    if ( cinfo->image_width != mWidth || cinfo->image_height != mHeight || newFormat != mFormat )
    {
        assign( newFormat, cinfo->image_width, cinfo->image_height );
        if ( !(cinfo->num_components == 1 || cinfo->num_components == 3) )
        {
            Error( "Unexpected channels (%d) when reading jpeg image", mChannels );
            jpeg_abort_decompress( cinfo );
            fclose( infile );
            return( false );
        }
    }

    Debug( 3, "Reading JPEG %d->%d format image into %d format, filename %s", cinfo->jpeg_color_space, cinfo->out_color_space, mFormat, filename.c_str() );
    jpeg_start_decompress( cinfo );

    JSAMPROW rowPointer;    /* pointer to a single row */
    int rowStride = mWidth * mChannels; /* physical row width in buffer */
    while ( cinfo->output_scanline < cinfo->output_height )
    {
        rowPointer = mBuffer.data()+(cinfo->output_scanline * rowStride);
        jpeg_read_scanlines( cinfo, &rowPointer, 1 );
    }

    jpeg_finish_decompress( cinfo );

    //jpeg_destroy_decompress( cinfo );

    fclose( infile );

    return( true );
}

/**
* @brief 
*
* @param filename
* @param qualityOverride
*
* @return 
*/
bool Image::writeJpeg( const std::string &filename, int qualityOverride ) const
{
    if ( qualityOverride < 0 || qualityOverride > 100 )
        Fatal( "Out of range quality %d when writing jpeg", qualityOverride );

    J_COLOR_SPACE inColourSpace = JCS_UNKNOWN;
    switch( mFormat )
    {
        case FMT_GREY :
        {
            if ( ALLOW_JPEG_GREY_COLOURSPACE )
                inColourSpace = JCS_GRAYSCALE; /* colorspace of input image */
            break;
        }
        case FMT_RGB :
        {
            inColourSpace = JCS_RGB; /* colorspace of input image */
            break;
        }
        case FMT_YUV :
        {
            Warning( "Writing JPEG image %s from YUV image, CS ranges are limited", filename.c_str() );
            // Fallthru
        }
        case FMT_YUVJ :
        {
            inColourSpace = JCS_YCbCr; /* colorspace of input image */
            break;
        }
        default :
        {
        }
    }

    if ( inColourSpace == JCS_UNKNOWN )
    {
        Debug( 1, "No direct JPEG colourspace for format %d, using temporary image", mFormat );
        if ( mFormat == FMT_GREY16 )
        {
            Image tempImage( FMT_GREY, *this );
            return( tempImage.writeJpeg( filename.c_str(), qualityOverride ) );
        }
        else
        {
            if ( false )
            {
                FILE *fp = fopen( "/tmp/yyy.data", "w" );
                fwrite( mBuffer.data(), mBuffer.size(), 1, fp );
                fclose( fp );
                //exit( 0 );
            }
            Image tempImage( FMT_YUVJ, *this );
            return( tempImage.writeJpeg( filename.c_str(), qualityOverride ) );
        }
    }

    ScopedMutex mutex( smJpegCoMutex );

    int quality = qualityOverride?qualityOverride:JPEG_QUALITY;

    struct jpeg_compress_struct *cinfo = jpegCcInfo( quality );

    FILE *outfile;
    if ( (outfile = fopen( filename.c_str(), "wb" )) == NULL )
    {
        Error( "Can't open %s: %s", filename.c_str(), strerror(errno) );
        return( false );
    }

    if ( setjmp( smJpegCoError.setjmp_buffer ) )
    {
        jpeg_abort_compress( cinfo );
        fclose( outfile );
        return( false );
    }

    jpeg_stdio_dest( cinfo, outfile );

    cinfo->image_width = mWidth;    /* image width and height, in pixels */
    cinfo->image_height = mHeight;
    cinfo->input_components = mChannels;    /* # of color components per pixel */
    cinfo->in_color_space = inColourSpace;

    jpeg_set_defaults( cinfo );
    jpeg_set_quality( cinfo, quality, false );
    cinfo->dct_method = JDCT_FASTEST;

    Debug( 3, "Writing %d format image as JPEG %d->%d format, quality %d, filename %s", mFormat, cinfo->in_color_space, cinfo->jpeg_color_space, quality, filename.c_str() );
    jpeg_start_compress( cinfo, TRUE );
    if ( ADD_JPEG_COMMENTS && !mText.empty() )
        jpeg_write_marker( cinfo, JPEG_COM, (const JOCTET *)mText.c_str(), mText.length() );

    JSAMPROW rowPointer;    /* pointer to a single row */
    int rowStride = cinfo->image_width * cinfo->input_components;   /* physical row width in buffer */
    while ( cinfo->next_scanline < cinfo->image_height )
    {
        rowPointer = mBuffer.data()+(cinfo->next_scanline * rowStride);
        jpeg_write_scanlines( cinfo, &rowPointer, 1 );
    }

    jpeg_finish_compress( cinfo );

    //jpeg_destroy_compress( cinfo );

    fclose( outfile );

    return( true );
}

/**
* @brief 
*
* @param inbuffer
* @param inbufferSize
*
* @return 
*/
bool Image::decodeJpeg( const JOCTET *inbuffer, int inbufferSize )
{
    ScopedMutex mutex( smJpegDeMutex );

    struct jpeg_decompress_struct *cinfo = jpegDcInfo();

    if ( setjmp( smJpegDeError.setjmp_buffer ) )
    {
        jpeg_abort_decompress( cinfo );
        return( false );
    }

    local_jpeg_mem_src( cinfo, (JOCTET *)inbuffer, inbufferSize );

    jpeg_read_header( cinfo, TRUE );

    J_COLOR_SPACE outColourSpace = JCS_UNKNOWN;
    switch( mFormat )
    {
        case FMT_GREY :
            // Jpeglib can't do RGB->Greyscale on input
            if ( cinfo->jpeg_color_space != JCS_RGB )
                outColourSpace = JCS_GRAYSCALE;
            break;
        case FMT_RGB :
            outColourSpace = JCS_RGB;
            break;
        case FMT_YUVJ :
            if ( cinfo->jpeg_color_space != JCS_RGB )
                outColourSpace = JCS_YCbCr;
            break;
        default :
            break;
    }

    Format newFormat = mFormat;
    if ( outColourSpace != JCS_UNKNOWN )
        cinfo->out_color_space = outColourSpace;
    else
    {
        switch( cinfo->out_color_space )
        {
            case JCS_GRAYSCALE :
            {
                newFormat = FMT_GREY;
                break;
            }
            case JCS_RGB :
            {
                newFormat = FMT_RGB;
                break;
            }
            case JCS_YCbCr :
            {
                newFormat = FMT_YUVJ;
                break;
            }
            case JCS_UNKNOWN :
            case JCS_CMYK :
            case JCS_YCCK :
            default :
            {
                Error( "Unsupported colourspace %d found when decoding JPEG", cinfo->jpeg_color_space );
                jpeg_abort_decompress( cinfo );
                return( false );
            }
        }
    }

    if ( cinfo->image_width != mWidth || cinfo->image_height != mHeight || newFormat != mFormat )
    {
        assign( newFormat, cinfo->image_width, cinfo->image_height );
        if ( !(cinfo->num_components == 1 || cinfo->num_components == 3) )
        {
            Error( "Unexpected channels (%d) when decoding jpeg image", mChannels );
            jpeg_abort_decompress( cinfo );
            return( false );
        }
    }

    Debug( 3, "Decoding JPEG %d->%d format image into format %d", cinfo->jpeg_color_space, cinfo->out_color_space, mFormat );
    jpeg_start_decompress( cinfo );

    JSAMPROW rowPointer;    /* pointer to a single row */
    int rowStride = mWidth * mChannels; /* physical row width in buffer */
    while ( cinfo->output_scanline < cinfo->output_height )
    {
        rowPointer = mBuffer.data()+(cinfo->output_scanline * rowStride);
        jpeg_read_scanlines( cinfo, &rowPointer, 1 );
    }

    jpeg_finish_decompress( cinfo );

    //jpeg_destroy_decompress( cinfo );

    return( true );
}

/**
* @brief 
*
* @param outbuffer
* @param outbufferSize
* @param qualityOverride
*
* @return 
*/
bool Image::encodeJpeg( JOCTET *outbuffer, int *outbufferSize, int qualityOverride ) const
{
    if ( qualityOverride < 0 || qualityOverride > 100 )
        Fatal( "Out of range quality %d when writing jpeg", qualityOverride );

    J_COLOR_SPACE inColourSpace = JCS_UNKNOWN;
    switch( mFormat )
    {
        case FMT_GREY :
        {
            if ( ALLOW_JPEG_GREY_COLOURSPACE )
                inColourSpace = JCS_GRAYSCALE; /* colorspace of input image */
            break;
        }
        case FMT_RGB :
        {
            inColourSpace = JCS_RGB; /* colorspace of input image */
            break;
        }
        case FMT_YUV :
        {
            Warning( "Encoding JPEG image from YUV image, CS ranges are limited" );
            // Fallthru
        }
        case FMT_YUVJ :
        {
            inColourSpace = JCS_YCbCr; /* colorspace of input image */
            break;
        }
        default :
        {
            // Empty
        }
    }

    if ( inColourSpace == JCS_UNKNOWN )
    {
        Debug( 1, "No direct JPEG colourspace for format %d, using temporary image", mFormat );
        if ( mFormat == FMT_GREY16 )
        {
            Image tempImage( FMT_GREY, *this );
            return( tempImage.encodeJpeg( outbuffer, outbufferSize, qualityOverride ) );
        }
        else
        {
            Image tempImage( FMT_YUVJ, *this );
            return( tempImage.encodeJpeg( outbuffer, outbufferSize, qualityOverride ) );
        }
    }

    ScopedMutex mutex( smJpegCoMutex );

    int quality = qualityOverride?qualityOverride:JPEG_QUALITY;

    struct jpeg_compress_struct *cinfo = jpegCcInfo( quality );

    if ( setjmp( smJpegCoError.setjmp_buffer ) )
    {
        jpeg_abort_compress( cinfo );
        return( false );
    }

    local_jpeg_mem_dest( cinfo, (JOCTET *)outbuffer, outbufferSize );

    cinfo->image_width = mWidth;    /* image width and height, in pixels */
    cinfo->image_height = mHeight;
    cinfo->input_components = mChannels;    /* # of color components per pixel */
    cinfo->in_color_space = inColourSpace;

    jpeg_set_defaults( cinfo );
    jpeg_set_quality( cinfo, quality, false );
    cinfo->dct_method = JDCT_FASTEST;

    Debug( 3, "Encoding %d format image as JPEG %d->%d format, quality %d", mFormat, cinfo->in_color_space, cinfo->jpeg_color_space, quality );
    jpeg_start_compress( cinfo, TRUE );
    if ( ADD_JPEG_COMMENTS && !mText.empty() )
        jpeg_write_marker( cinfo, JPEG_COM, (const JOCTET *)mText.c_str(), mText.length() );

    JSAMPROW rowPointer;    /* pointer to a single row */
    int rowStride = cinfo->image_width * cinfo->input_components;   /* physical row width in buffer */
    while ( cinfo->next_scanline < cinfo->image_height )
    {
        rowPointer = mBuffer.data()+(cinfo->next_scanline * rowStride);
        jpeg_write_scanlines( cinfo, &rowPointer, 1 );
    }

    jpeg_finish_compress( cinfo );

    //jpeg_destroy_compress( cinfo );

    return( true );
}

#if HAVE_ZLIB_H
/**
* @brief 
*
* @param inbuffer
* @param inbufferSize
*
* @return 
*/
bool Image::unzip( const Bytef *inbuffer, unsigned long inbufferSize )
{
    unsigned long zipSize = mBuffer.size();
    int result = uncompress( mBuffer.data(), &zipSize, inbuffer, inbufferSize );
    if ( result != Z_OK )
    {
        Error( "Unzip failed, result = %d", result );
        return( false );
    }
    if ( zipSize != mBuffer.size() )
    {
        Error( "Unzip failed, size mismatch, expected %lu bytes, got %ld", mBuffer.size(), zipSize );
        return( false );
    }
    return( true );
}

/**
* @brief 
*
* @param outbuffer
* @param outbufferSize
* @param compression_level
*
* @return 
*/
bool Image::zip( Bytef *outbuffer, unsigned long *outbufferSize, int compression_level ) const
{
    int result = compress2( outbuffer, outbufferSize, mBuffer.data(), mBuffer.size(), compression_level );
    if ( result != Z_OK )
    {
        Error( "Zip failed, result = %d", result );
        return( false );
    }
    return( true );
}
#endif // HAVE_ZLIB_H

/**
* @brief 
*
* @param loX
* @param loY
* @param hiX
* @param hiY
*
* @return 
*/
bool Image::crop( int loX, int loY, int hiX, int hiY )
{
    int newWidth = (hiX-loX)+1;
    int newHeight = (hiY-loY)+1;

    if ( loX > hiX || loY > hiY )
    {
        Error( "Invalid or reversed crop region %d,%d -> %d,%d", loX, loY, hiX, hiY );
        return( false );
    }
    if ( loX < 0 || hiX > (mWidth-1) || ( loY < 0 || hiY > (mHeight-1) ) )
    {
        Error( "Attempting to crop outside image, %d,%d -> %d,%d not in %d,%d", loX, loY, hiX, hiY, mWidth-1, mHeight-1 );
        return( false );
    }

    if ( newWidth == mWidth && newHeight == mHeight )
    {
        return( true );
    }

    Image cropImage( mFormat, newWidth, newHeight );

    unsigned char *bufferPtr = mBuffer.data();
    unsigned char *cropBufferPtr = cropImage.mBuffer.data();
    for ( int c = 0; c < mPlanes; c++ )
    {
        for ( int y = loY, ny = 0; y <= hiY; y++, ny++ )
        {
            unsigned char *pbuf = &bufferPtr[((y*mWidth)+loX)*mPixelStep];
            unsigned char *pnbuf = &cropBufferPtr[(ny*cropImage.mWidth)*cropImage.mPixelStep];
            memcpy( pnbuf, pbuf, cropImage.mStride );
        }
        bufferPtr += mPlaneSize;
        cropBufferPtr += (cropImage.mStride*cropImage.mHeight);
    }

    assign( cropImage );

    return( true );
}

/**
* @brief 
*
* @param limits
*
* @return 
*/
bool Image::crop( const Box &limits )
{
    return( crop( limits.loX(), limits.loY(), limits.hiX(), limits.hiY() ) );
}

// Set all image to all non-zero pixels of passed image
/**
* @brief 
*
* @param image
*/
void Image::overlay( const Image &image )
{
    if ( !(mWidth == image.mWidth && mHeight == image.mHeight) )
        Panic( "Attempt to overlay different sized images, expected %dx%d, got %dx%d", mWidth, mHeight, image.mWidth, image.mHeight );
    if ( mFormat == FMT_UNDEF || image.mFormat == FMT_UNDEF )
        Panic( "Attempt to overlay to or from image with undefined format" );

    unsigned char *pSrc = image.mBuffer.data();
    unsigned char *pDst = mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                case FMT_YUVJ :
                case FMT_YUVJP :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                            *pDst = *pSrc;
                        pDst++;
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( RED(pSrc) || GREEN(pSrc) || BLUE(pSrc) )
                            *pDst = RGB2YUVJ_Y(*pSrc,*(pSrc+image.mPixelWidth),*(pSrc+(2*image.mPixelWidth)));
                        pDst++;
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                case FMT_YUVP :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc > 16 )
                            *pDst = YUV2YUVJ_Y(*pSrc);
                        pDst++;
                        pSrc += image.mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_GREY16 :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_YUVJ :
                case FMT_YUVJP :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDst = *pSrc;
                            *(pDst+1) = 0;
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_GREY16 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc || *(pSrc+1) )
                        {
                            *pDst = *pSrc;
                            *(pDst+1) = *(pSrc+1);
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( RED(pSrc) || GREEN(pSrc) || BLUE(pSrc) )
                        {
                            *pDst = RGB2YUVJ_Y(*pSrc,*(pSrc+image.mPixelWidth),*(pSrc+(2*image.mPixelWidth)));
                            *(pDst+1) = 0;
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                case FMT_YUVP :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc > 16 )
                        {
                            *pDst = YUV2YUVJ_Y(*pSrc);
                            *(pDst+1) = 0;
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_RGB :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                            RED(pDst) = GREEN(pDst) = BLUE(pDst) = *pSrc;
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( RED(pSrc) || GREEN(pSrc) || BLUE(pSrc) )
                        {
                            RED(pDst) = RED(pSrc);
                            GREEN(pDst) = GREEN(pSrc);
                            BLUE(pDst) = BLUE(pSrc);
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc || *(pSrc+2) || *(pSrc+4) )
                        {
                            *pDst = *pSrc;
                            *(pDst+1) = *(pSrc+2);
                            *(pDst+2) = *(pSrc+4);
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc > 16 )
                        {
                            RED(pDst) = YUV2RGB_R(*(pSrc),*(pSrc+1),*(pSrc+2));
                            GREEN(pDst) = YUV2RGB_G(*(pSrc),*(pSrc+1),*(pSrc+2));
                            BLUE(pDst) = YUV2RGB_B(*(pSrc),*(pSrc+1),*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            RED(pDst) = YUVJ2RGB_R(*(pSrc),*(pSrc+1),*(pSrc+2));
                            GREEN(pDst) = YUVJ2RGB_G(*(pSrc),*(pSrc+1),*(pSrc+2));
                            BLUE(pDst) = YUVJ2RGB_B(*(pSrc),*(pSrc+1),*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrcY > 16 )
                        {
                            RED(pDst) = YUV2RGB_R(*pSrcY,*pSrcU,*pSrcV);
                            GREEN(pDst) = YUV2RGB_G(*pSrcY,*pSrcU,*pSrcV);
                            BLUE(pDst) = YUV2RGB_B(*pSrcY,*pSrcU,*pSrcV);
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrcY )
                        {
                            RED(pDst) = YUVJ2RGB_R(*pSrcY,*pSrcU,*pSrcV);
                            GREEN(pDst) = YUVJ2RGB_G(*pSrcY,*pSrcU,*pSrcV);
                            BLUE(pDst) = YUVJ2RGB_B(*pSrcY,*pSrcU,*pSrcV);
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_RGB48 :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDst = *(pDst+2) = *(pDst+4) = *pSrc;
                            *(pDst+1) = *(pDst+3) = *(pDst+5) = 0;
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc || *(pSrc+1) )
                        {
                            *pDst = *(pDst+2) = *(pDst+4) = *pSrc;
                            *(pDst+1) = *(pDst+3) = *(pDst+5) = *(pSrc+1);
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( RED(pSrc) || GREEN(pSrc) || BLUE(pSrc) )
                        {
                            RED(pDst) = RED(pSrc);
                            GREEN(pDst) = GREEN(pSrc);
                            BLUE(pDst) = BLUE(pSrc);
                            *pDst = *pSrc;
                            *(pDst+2) = *(pSrc+1);
                            *(pDst+4) = *(pSrc+2);
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc || *(pSrc+1) || *(pSrc+2) || *(pSrc+3) || *(pSrc+4) || *(pSrc+5) )
                        {
                            memcpy( pDst, pSrc, mPixelStep );
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc > 16 )
                        {
                            *(pDst) = YUV2RGB_R(*(pSrc),*(pSrc+1),*(pSrc+2));
                            *(pDst+2) = YUV2RGB_G(*(pSrc),*(pSrc+1),*(pSrc+2));
                            *(pDst+4) = YUV2RGB_B(*(pSrc),*(pSrc+1),*(pSrc+2));
                            *(pDst+1) = *(pDst+3) = *(pDst+5) = 0;
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *(pDst) = YUVJ2RGB_R(*(pSrc),*(pSrc+1),*(pSrc+2));
                            *(pDst+2) = YUVJ2RGB_G(*(pSrc),*(pSrc+1),*(pSrc+2));
                            *(pDst+4) = YUVJ2RGB_B(*(pSrc),*(pSrc+1),*(pSrc+2));
                            *(pDst+1) = *(pDst+3) = *(pDst+5) = 0;
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrcY > 16 )
                        {
                            *(pDst) = YUV2RGB_R(*pSrcY,*pSrcU,*pSrcV);
                            *(pDst+2) = YUV2RGB_G(*pSrcY,*pSrcU,*pSrcV);
                            *(pDst+4) = YUV2RGB_B(*pSrcY,*pSrcU,*pSrcV);
                            *(pDst+1) = *(pDst+3) = *(pDst+5) = 0;
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrcY )
                        {
                            *(pDst) = YUVJ2RGB_R(*pSrcY,*pSrcU,*pSrcV);
                            *(pDst+2) = YUVJ2RGB_G(*pSrcY,*pSrcU,*pSrcV);
                            *(pDst+4) = YUVJ2RGB_B(*pSrcY,*pSrcU,*pSrcV);
                            *(pDst+1) = *(pDst+3) = *(pDst+5) = 0;
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUV :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDst = YUVJ2YUV_Y(*pSrc);
                            *(pDst+1) = -128;
                            *(pDst+2) = -128;
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( RED(pSrc) || GREEN(pSrc) || BLUE(pSrc) )
                        {
                            *pDst = RGB2YUV_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                            *(pDst+1) = RGB2YUV_U(*pSrc,*(pSrc+1),*(pSrc+2));
                            *(pDst+2) = RGB2YUV_V(*pSrc,*(pSrc+1),*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( RED16(pSrc) || GREEN16(pSrc) || BLUE16(pSrc) )
                        {
                            *pDst = RGB2YUV_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                            *(pDst+1) = RGB2YUV_U(*pSrc,*(pSrc+2),*(pSrc+4));
                            *(pDst+2) = RGB2YUV_V(*pSrc,*(pSrc+2),*(pSrc+4));
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc > 16 )
                        {
                            *pDst = *(pSrc);
                            *(pDst+1) = *(pSrc+1);
                            *(pDst+2) = *(pSrc+2);
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDst = YUVJ2YUV_Y(*(pSrc));
                            *(pDst+1) = YUVJ2YUV_U(*(pSrc+1));
                            *(pDst+2) = YUVJ2YUV_V(*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrcY > 16 )
                        {
                            *pDst = *pSrcY;
                            *(pDst+1) = *pSrcU;
                            *(pDst+2) = *pSrcV;
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrcY )
                        {
                            *pDst = YUVJ2YUV_Y(*pSrcY);
                            *(pDst+1) = YUVJ2YUV_U(*pSrcU);
                            *(pDst+2) = YUVJ2YUV_V(*pSrcV);
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVJ :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    // Only copy over the top byte, for grey 16
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDst = *pSrc;
                            *(pDst+1) = -128;
                            *(pDst+2) = -128;
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( RED(pSrc) || GREEN(pSrc) || BLUE(pSrc) )
                        {
                            *pDst = RGB2YUVJ_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                            *(pDst+1) = RGB2YUVJ_U(*pSrc,*(pSrc+1),*(pSrc+2));
                            *(pDst+2) = RGB2YUVJ_V(*pSrc,*(pSrc+1),*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( RED16(pSrc) || GREEN16(pSrc) || BLUE16(pSrc) )
                        {
                            *pDst = RGB2YUVJ_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                            *(pDst+1) = RGB2YUVJ_U(*pSrc,*(pSrc+2),*(pSrc+4));
                            *(pDst+2) = RGB2YUVJ_V(*pSrc,*(pSrc+2),*(pSrc+4));
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc > 16 )
                        {
                            *pDst = YUV2YUVJ_Y(*(pSrc));
                            *(pDst+1) = YUV2YUVJ_U(*(pSrc+1));
                            *(pDst+2) = YUV2YUVJ_V(*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDst = *(pSrc);
                            *(pDst+1) = *(pSrc+1);
                            *(pDst+2) = *(pSrc+2);
                        }
                        pSrc += image.mPixelStep;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrcY > 16 )
                        {
                            *pDst = YUV2YUVJ_Y(*pSrcY);
                            *(pDst+1) = YUV2YUVJ_U(*pSrcU);
                            *(pDst+2) = YUV2YUVJ_V(*pSrcV);
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    while( pDst < mBuffer.tail() )
                    {
                        if ( *pSrcY )
                        {
                            *pDst = *pSrcY;
                            *(pDst+1) = *pSrcU;
                            *(pDst+2) = *pSrcV;
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDst += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVP :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDstY = YUVJ2YUV_Y(*pSrc);
                            *pDstU = -128;
                            *pDstV = -128;
                        }
                        pSrc++;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_GREY16 :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDstY = YUVJ2YUV_Y(*pSrc);
                            *pDstU = -128;
                            *pDstV = -128;
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( RED(pSrc) || GREEN(pSrc) || BLUE(pSrc) )
                        {
                            *pDstY = RGB2YUV_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                            *pDstU = RGB2YUV_U(*pSrc,*(pSrc+1),*(pSrc+2));
                            *pDstV = RGB2YUV_V(*pSrc,*(pSrc+1),*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( RED16(pSrc) || GREEN16(pSrc) || BLUE16(pSrc) )
                        {
                            *pDstY = RGB2YUV_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                            *pDstU = RGB2YUV_U(*pSrc,*(pSrc+2),*(pSrc+4));
                            *pDstV = RGB2YUV_V(*pSrc,*(pSrc+2),*(pSrc+4));
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( *pSrc > 16 )
                        {
                            *pDstY = *(pSrc);
                            *pDstU = *(pSrc+1);
                            *pDstV = *(pSrc+2);
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDstY = YUVJ2YUV_Y(*(pSrc));
                            *pDstU = YUVJ2YUV_U(*(pSrc+1));
                            *pDstV = YUVJ2YUV_V(*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrcV < image.mBuffer.tail() )
                    {
                        if ( *pSrcY > 16 )
                        {
                            *pDstY = *pSrcY;
                            *pDstU = *pSrcU;
                            *pDstV = *pSrcV;
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrcV < image.mBuffer.tail() )
                    {
                        if ( *pSrcY )
                        {
                            *pDstY = YUVJ2YUV_Y(*pSrcY);
                            *pDstU = YUVJ2YUV_U(*pSrcU);
                            *pDstV = YUVJ2YUV_V(*pSrcV);
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVJP :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDstY = *pSrc;
                            *pDstU = -128;
                            *pDstV = -128;
                        }
                        pSrc++;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_GREY16 :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDstY = *pSrc;
                            *pDstU = -128;
                            *pDstV = -128;
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( RED(pSrc) || GREEN(pSrc) || BLUE(pSrc) )
                        {
                            *pDstY = RGB2YUVJ_Y(*pSrc,*(pSrc+1),*(pSrc+2));
                            *pDstU = RGB2YUVJ_U(*pSrc,*(pSrc+1),*(pSrc+2));
                            *pDstV = RGB2YUVJ_V(*pSrc,*(pSrc+1),*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( RED16(pSrc) || GREEN16(pSrc) || BLUE16(pSrc) )
                        {
                            *pDstY = RGB2YUVJ_Y(*pSrc,*(pSrc+2),*(pSrc+4));
                            *pDstU = RGB2YUVJ_U(*pSrc,*(pSrc+2),*(pSrc+4));
                            *pDstV = RGB2YUVJ_V(*pSrc,*(pSrc+2),*(pSrc+4));
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( *pSrc > 16 )
                        {
                            *pDstY = YUV2YUVJ_Y(*(pSrc));
                            *pDstU = YUV2YUVJ_U(*(pSrc+1));
                            *pDstV = YUV2YUVJ_V(*(pSrc+2));
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrc < image.mBuffer.tail() )
                    {
                        if ( *pSrc )
                        {
                            *pDstY = *(pSrc);
                            *pDstU = *(pSrc+1);
                            *pDstV = *(pSrc+2);
                        }
                        pSrc += image.mPixelStep;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrcV < image.mBuffer.tail() )
                    {
                        if ( *pSrcY > 16 )
                        {
                            *pDstY = YUV2YUVJ_Y(*pSrcY);
                            *pDstU = YUV2YUVJ_U(*pSrcU);
                            *pDstV = YUV2YUVJ_V(*pSrcV);
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pSrcY = pSrc;
                    unsigned char *pSrcU = pSrcY+(image.mStride*image.mHeight);
                    unsigned char *pSrcV = pSrcU+(image.mStride*image.mHeight);
                    unsigned char *pDstY = pDst;
                    unsigned char *pDstU = pDstY+mPlaneSize;
                    unsigned char *pDstV = pDstU+mPlaneSize;
                    while( pSrcV < image.mBuffer.tail() )
                    {
                        if ( *pSrcY )
                        {
                            *pDstY = *pSrcY;
                            *pDstU = *pSrcU;
                            *pDstV = *pSrcV;
                        }
                        pSrcY++;
                        pSrcU++;
                        pSrcV++;
                        pDstY++;
                        pDstU++;
                        pDstV++;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_UNDEF :
            break;
    }
    Panic( "Image with format %d does not support overlaying with images of format %d", mFormat, image.mFormat );
}

#if 0
void Image::overlay( const Image &image, int x, int y )
{
    if ( !(mWidth < image.mWidth || mHeight < image.mHeight) )
    {
        Panic( "Attempt to overlay image too big for destination, %dx%d > %dx%d", image.mWidth, image.mHeight, mWidth, mHeight );
    }

    if ( !(mWidth < (x+image.mWidth) || mHeight < (y+image.mHeight)) )
    {
        Panic( "Attempt to overlay image outside of destination bounds, %dx%d @ %dx%d > %dx%d", image.mWidth, image.mHeight, x, y, mWidth, mHeight );
    }

    if ( !(mChannels == image.mChannels) )
    {
        Panic( "Attempt to partial overlay differently coloured images, expected %d, got %d", mChannels, image.mChannels );
    }

    int loX = x;
    int loY = y;
    int hiX = (x+image.mWidth)-1;
    int hiY = (y+image.mHeight-1);
    if ( mChannels == 1 )
    {
        unsigned char *pSrc = image.buffer;
        for ( int y = loY; y <= hiY; y++ )
        {
            unsigned char *pDst = &buffer[(y*mWidth)+loX];
            for ( int x = loX; x <= hiX; x++ )
            {
                *pDst++ = *pSrc++;
            }
        }
    }
    else if ( mChannels == 3 )
    {
        unsigned char *pSrc = image.buffer;
        for ( int y = loY; y <= hiY; y++ )
        {
            unsigned char *pDst = &buffer[mChannels*((y*mWidth)+loX)];
            for ( int x = loX; x <= hiX; x++ )
            {
                *pDst++ = *pSrc++;
                *pDst++ = *pSrc++;
                *pDst++ = *pSrc++;
            }
        }
    }
}
#endif

#if 0
void Image::blend( const Image &image, int transparency ) const
{
    if ( !(mWidth == image.mWidth && mHeight == image.mHeight) )
    {
        Panic( "Attempt to blend different sized images, expected %dx%d, got %dx%d", mWidth, mHeight, image.mWidth, image.mHeight );
    }

    BlendTablePtr blendPtr = getBlendTable( transparency );

    unsigned char *pSrc = image.mBuffer.head();
    unsigned char *pDst = mBuffer.head();
    while( pDst < mBuffer.tail() )
    {
        *pDst++ = (*blendPtr)[*pDst][*pSrc++];
    }
}
#endif

/**
* @brief 
*
* @param image
* @param fraction
*/
void Image::blend( const Image &image, unsigned short fraction ) const
{
    if ( !(mWidth == image.mWidth && mHeight == image.mHeight) )
        Panic( "Attempt to blend different sized images, expected %dx%d, got %dx%d", mWidth, mHeight, image.mWidth, image.mHeight );
    if ( mFormat == FMT_UNDEF || image.mFormat == FMT_UNDEF )
        Panic( "Attempt to blend image with undefined format" );

    int refShift = getShift( fraction );
    if ( refShift == 0 )
        Fatal( "Blend fraction must be power of two, %d not valid", fraction );
    int imgShift = refShift + (image.mColourDepth-mColourDepth);
    if ( fraction > 16 && (mFormat != FMT_GREY16 && mFormat != FMT_RGB48) )
        Warning( "Attempt to blend image of format %d with fraction %d, too high", mFormat, fraction );
    if ( refShift >= mColourDepth )
    {
        Warning( "Reference image shift of %d bits will have no effect, not blending", refShift );
        return;
    }
    Debug( 5, "Blending format %d onto format %d with fraction %d, refshift %d, imgshift %d", image.mFormat, mFormat, fraction, refShift, imgShift );
    unsigned char *pRef = mBuffer.data();
    unsigned char *pImg = image.mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY :
        {
            switch( image.mFormat )
            {
                case FMT_GREY16 :
                    imgShift -= 8;
                    // Fallthru
                case FMT_GREY :
                case FMT_YUVJ :
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (RGB2YUVJ_Y(*(pImg),*(pImg+1),*(pImg+2))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (RGB2YUVJ_Y(*(pImg),*(pImg+2),*(pImg+4))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUV2YUVJ_Y(*pImg)>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_GREY16 :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_YUVJ :
                case FMT_YUVJP :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            ref = ref - (ref>>refShift) + (((unsigned short)*pImg)<<imgShift);
                            *pRef = (unsigned char)(ref >> 8);
                            *(pRef+1) = (unsigned char)ref;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    else
                    {
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            ref = ref - (ref>>refShift) + (((unsigned short)*pImg)>>imgShift);
                            *pRef = (unsigned char)(ref >> 8);
                            *(pRef+1) = (unsigned char)ref;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                        unsigned short img = (((unsigned short)*pImg)<<8) + *(pImg+1);
                        ref = ref - (ref>>refShift) + (img>>imgShift);
                        *pRef = (unsigned char)(ref >> 8);
                        *(pRef+1) = (unsigned char)ref;
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            ref = ref - (ref>>refShift) + (((unsigned short)RGB2YUVJ_Y(*(pImg),*(pImg+1),*(pImg+2)))<<imgShift);
                            *pRef = (unsigned char)(ref >> 8);
                            *(pRef+1) = (unsigned char)ref;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    else
                    {
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            ref = ref - (ref>>refShift) + (((unsigned short)RGB2YUVJ_Y(*(pImg),*(pImg+1),*(pImg+2)))>>imgShift);
                            *pRef = (unsigned char)(ref >> 8);
                            *(pRef+1) = (unsigned char)ref;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    imgShift -= 8;
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            ref = ref - (ref>>refShift) + (((unsigned short)RGB2YUVJ_Y(*(pImg),*(pImg+2),*(pImg+4)))<<imgShift);
                            *pRef = (unsigned char)(ref >> 8);
                            *(pRef+1) = (unsigned char)ref;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    else
                    {
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            ref = ref - (ref>>refShift) + (((unsigned short)RGB2YUVJ_Y(*(pImg),*(pImg+2),*(pImg+4)))>>imgShift);
                            *pRef = (unsigned char)(ref >> 8);
                            *(pRef+1) = (unsigned char)ref;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_YUV :
                case FMT_YUVP :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            ref = ref - (ref>>refShift) + ((unsigned short)(YUV2YUVJ_Y(*(pImg)))<<imgShift);
                            *pRef = (unsigned char)(ref >> 8);
                            *(pRef+1) = (unsigned char)ref;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    else
                    {
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            ref = ref - (ref>>refShift) + ((unsigned short)(YUV2YUVJ_Y(*(pImg)))>>imgShift);
                            *pRef = (unsigned char)(ref >> 8);
                            *(pRef+1) = (unsigned char)ref;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_RGB :
        {
            switch( image.mFormat )
            {
                case FMT_GREY16 :
                    imgShift -= 8;
                    // Fallthru
                case FMT_GREY :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (*pImg>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (*pImg>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (*(pImg+1)>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (*(pImg+2)>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    imgShift -= 8;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (*(pImg+2)>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (*(pImg+4)>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUV2RGB_R(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (YUV2RGB_G(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (YUV2RGB_B(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUVJ2RGB_R(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (YUVJ2RGB_G(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (YUVJ2RGB_B(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pImgY = pImg;
                    unsigned char *pImgU = pImgY+image.mPlaneSize;
                    unsigned char *pImgV = pImgU+image.mPlaneSize;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUV2RGB_R(*pImgY,*pImgU,*pImgV)>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (YUV2RGB_G(*pImgY,*pImgU,*pImgV)>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (YUV2RGB_B(*pImgY,*pImgU,*pImgV)>>imgShift);
                        pRef += mPixelStep;
                        pImgY += image.mPixelStep;
                        pImgU += image.mPixelStep;
                        pImgV += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pImgY = pImg;
                    unsigned char *pImgU = pImgY+image.mPlaneSize;
                    unsigned char *pImgV = pImgU+image.mPlaneSize;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUVJ2RGB_R(*pImgY,*pImgU,*pImgV)>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (YUVJ2RGB_G(*pImgY,*pImgU,*pImgV)>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (YUVJ2RGB_B(*pImgY,*pImgU,*pImgV)>>imgShift);
                        pRef += mPixelStep;
                        pImgY += image.mPixelStep;
                        pImgU += image.mPixelStep;
                        pImgV += image.mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_RGB48 :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)*pImg)<<imgShift);
                            g = g - (g>>refShift) + (((unsigned short)*pImg)<<imgShift);
                            b = b - (b>>refShift) + (((unsigned short)*pImg)<<imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    else
                    {
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)*pImg)>>imgShift);
                            g = g - (g>>refShift) + (((unsigned short)*pImg)>>imgShift);
                            b = b - (b>>refShift) + (((unsigned short)*pImg)>>imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                        unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                        unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                        unsigned short y = (((unsigned short)*pImg)<<8) + *(pImg+1);
                        r = r - (r>>refShift) + (y>>imgShift);
                        g = g - (g>>refShift) + (y>>imgShift);
                        b = b - (b>>refShift) + (y>>imgShift);
                        *pRef = (unsigned char)(r >> 8);
                        *(pRef+1) = (unsigned char)r;
                        *(pRef+2) = (unsigned char)(g >> 8);
                        *(pRef+3) = (unsigned char)g;
                        *(pRef+4) = (unsigned char)(b >> 8);
                        *(pRef+5) = (unsigned char)b;
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)*pImg)<<imgShift);
                            g = g - (g>>refShift) + (((unsigned short)*(pImg+1))<<imgShift);
                            b = b - (b>>refShift) + (((unsigned short)*(pImg+2))<<imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    else
                    {
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)*pImg)>>imgShift);
                            g = g - (g>>refShift) + (((unsigned short)*(pImg+1))>>imgShift);
                            b = b - (b>>refShift) + (((unsigned short)*(pImg+2))>>imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        unsigned short r1 = (((unsigned short)*pRef)<<8) + *(pRef+1);
                        unsigned short g1 = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                        unsigned short b1 = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                        unsigned short r2 = (((unsigned short)*pImg)<<8) + *(pImg+1);
                        unsigned short g2 = (((unsigned short)*(pImg+2))<<8) + *(pImg+3);
                        unsigned short b2 = (((unsigned short)*(pImg+4))<<8) + *(pImg+5);
                        r1 = r1 - (r1>>refShift) + (r2>>imgShift);
                        g1 = g1 - (g1>>refShift) + (g2>>imgShift);
                        b1 = b1 - (b1>>refShift) + (b2>>imgShift);
                        *pRef = (unsigned char)(r1 >> 8);
                        *(pRef+1) = (unsigned char)r1;
                        *(pRef+2) = (unsigned char)(g1 >> 8);
                        *(pRef+3) = (unsigned char)g1;
                        *(pRef+4) = (unsigned char)(b1 >> 8);
                        *(pRef+5) = (unsigned char)b1;
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)YUV2RGB_R(*pImg,*(pImg+1),*(pImg+2)))<<imgShift);
                            g = g - (g>>refShift) + (((unsigned short)YUV2RGB_G(*pImg,*(pImg+1),*(pImg+2)))<<imgShift);
                            b = b - (b>>refShift) + (((unsigned short)YUV2RGB_B(*pImg,*(pImg+1),*(pImg+2)))<<imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    else
                    {
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)YUV2RGB_R(*pImg,*(pImg+1),*(pImg+2)))>>imgShift);
                            g = g - (g>>refShift) + (((unsigned short)YUV2RGB_G(*pImg,*(pImg+1),*(pImg+2)))>>imgShift);
                            b = b - (b>>refShift) + (((unsigned short)YUV2RGB_B(*pImg,*(pImg+1),*(pImg+2)))>>imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)YUVJ2RGB_R(*pImg,*(pImg+1),*(pImg+2)))<<imgShift);
                            g = g - (g>>refShift) + (((unsigned short)YUVJ2RGB_G(*pImg,*(pImg+1),*(pImg+2)))<<imgShift);
                            b = b - (b>>refShift) + (((unsigned short)YUVJ2RGB_B(*pImg,*(pImg+1),*(pImg+2)))<<imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    else
                    {
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)YUVJ2RGB_R(*pImg,*(pImg+1),*(pImg+2)))>>imgShift);
                            g = g - (g>>refShift) + (((unsigned short)YUVJ2RGB_G(*pImg,*(pImg+1),*(pImg+2)))>>imgShift);
                            b = b - (b>>refShift) + (((unsigned short)YUVJ2RGB_B(*pImg,*(pImg+1),*(pImg+2)))>>imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImg += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        unsigned char *pImgY = pImg;
                        unsigned char *pImgU = pImgY+image.mPlaneSize;
                        unsigned char *pImgV = pImgU+image.mPlaneSize;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)YUV2RGB_R(*pImgY,*pImgU,*pImgV))<<imgShift);
                            g = g - (g>>refShift) + (((unsigned short)YUV2RGB_G(*pImgY,*pImgU,*pImgV))<<imgShift);
                            b = b - (b>>refShift) + (((unsigned short)YUV2RGB_B(*pImgY,*pImgU,*pImgV))<<imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImgY += image.mPixelStep;
                            pImgU += image.mPixelStep;
                            pImgV += image.mPixelStep;
                        }
                    }
                    else
                    {
                        unsigned char *pImgY = pImg;
                        unsigned char *pImgU = pImgY+image.mPlaneSize;
                        unsigned char *pImgV = pImgU+image.mPlaneSize;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)YUV2RGB_R(*pImgY,*pImgU,*pImgV))>>imgShift);
                            g = g - (g>>refShift) + (((unsigned short)YUV2RGB_G(*pImgY,*pImgU,*pImgV))>>imgShift);
                            b = b - (b>>refShift) + (((unsigned short)YUV2RGB_B(*pImgY,*pImgU,*pImgV))>>imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImgY += image.mPixelStep;
                            pImgU += image.mPixelStep;
                            pImgV += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    if ( imgShift < 0 )
                    {
                        imgShift = -imgShift;
                        unsigned char *pImgY = pImg;
                        unsigned char *pImgU = pImgY+image.mPlaneSize;
                        unsigned char *pImgV = pImgU+image.mPlaneSize;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)YUVJ2RGB_R(*pImgY,*pImgU,*pImgV))<<imgShift);
                            g = g - (g>>refShift) + (((unsigned short)YUVJ2RGB_G(*pImgY,*pImgU,*pImgV))<<imgShift);
                            b = b - (b>>refShift) + (((unsigned short)YUVJ2RGB_B(*pImgY,*pImgU,*pImgV))<<imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImgY += image.mPixelStep;
                            pImgU += image.mPixelStep;
                            pImgV += image.mPixelStep;
                        }
                    }
                    else
                    {
                        unsigned char *pImgY = pImg;
                        unsigned char *pImgU = pImgY+image.mPlaneSize;
                        unsigned char *pImgV = pImgU+image.mPlaneSize;
                        while( pRef < mBuffer.tail() )
                        {
                            unsigned short r = (((unsigned short)*pRef)<<8) + *(pRef+1);
                            unsigned short g = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                            unsigned short b = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                            r = r - (r>>refShift) + (((unsigned short)YUVJ2RGB_R(*pImgY,*pImgU,*pImgV))>>imgShift);
                            g = g - (g>>refShift) + (((unsigned short)YUVJ2RGB_G(*pImgY,*pImgU,*pImgV))>>imgShift);
                            b = b - (b>>refShift) + (((unsigned short)YUVJ2RGB_B(*pImgY,*pImgU,*pImgV))>>imgShift);
                            *pRef = (unsigned char)(r >> 8);
                            *(pRef+1) = (unsigned char)r;
                            *(pRef+2) = (unsigned char)(g >> 8);
                            *(pRef+3) = (unsigned char)g;
                            *(pRef+4) = (unsigned char)(b >> 8);
                            *(pRef+5) = (unsigned char)b;
                            pRef += mPixelStep;
                            pImgY += image.mPixelStep;
                            pImgU += image.mPixelStep;
                            pImgV += image.mPixelStep;
                        }
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUV :
        {
            switch( image.mFormat )
            {
                case FMT_GREY16 :
                    imgShift -= 8;
                    // Fallthru
                case FMT_GREY :
                {
                    unsigned char uv = 128>>imgShift;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUVJ2YUV_Y(*pImg)>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + uv;
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + uv;
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (RGB2YUV_Y(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (RGB2YUV_U(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (RGB2YUV_V(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    imgShift -= 8;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (RGB2YUV_Y(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (RGB2YUV_U(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (RGB2YUV_V(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        pRef++;
                        pImg++;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUVJ2YUV_Y(*pImg)>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (YUVJ2YUV_U(*(pImg+1))>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (YUVJ2YUV_V(*(pImg+2))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pImgY = pImg;
                    unsigned char *pImgU = pImgY+image.mPlaneSize;
                    unsigned char *pImgV = pImgU+image.mPlaneSize;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImgY>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (*pImgU>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (*pImgV>>imgShift);
                        pRef += mPixelStep;
                        pImgY += image.mPixelStep;
                        pImgU += image.mPixelStep;
                        pImgV += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pImgY = pImg;
                    unsigned char *pImgU = pImgY+image.mPlaneSize;
                    unsigned char *pImgV = pImgU+image.mPlaneSize;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUVJ2YUV_Y(*pImgY)>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (YUVJ2YUV_U(*pImgU)>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (YUVJ2YUV_V(*pImgV)>>imgShift);
                        pRef += mPixelStep;
                        pImgY += image.mPixelStep;
                        pImgU += image.mPixelStep;
                        pImgV += image.mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVJ :
        {
            switch( image.mFormat )
            {
                case FMT_GREY16 :
                    imgShift -= 8;
                    // Fallthru
                case FMT_GREY :
                {
                    unsigned char uv = 128>>imgShift;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + uv;
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + uv;
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (RGB2YUVJ_Y(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (RGB2YUVJ_U(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (RGB2YUVJ_V(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    imgShift -= 8;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (RGB2YUVJ_Y(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (RGB2YUVJ_U(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (RGB2YUVJ_V(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUV2YUVJ_Y(*pImg)>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (YUV2YUVJ_U(*(pImg+1))>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (YUV2YUVJ_V(*(pImg+2))>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        pRef++;
                        pImg++;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pImgY = pImg;
                    unsigned char *pImgU = pImgY+image.mPlaneSize;
                    unsigned char *pImgV = pImgU+image.mPlaneSize;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (YUV2YUVJ_Y(*pImgY)>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (YUV2YUVJ_U(*pImgU)>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (YUV2YUVJ_V(*pImgV)>>imgShift);
                        pRef += mPixelStep;
                        pImgY += image.mPixelStep;
                        pImgU += image.mPixelStep;
                        pImgV += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pImgY = pImg;
                    unsigned char *pImgU = pImgY+image.mPlaneSize;
                    unsigned char *pImgV = pImgU+image.mPlaneSize;
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImgY>>imgShift);
                        *(pRef+1) = *(pRef+1) - (*(pRef+1)>>refShift) + (*pImgU>>imgShift);
                        *(pRef+2) = *(pRef+2) - (*(pRef+2)>>refShift) + (*pImgV>>imgShift);
                        pRef += mPixelStep;
                        pImgY += image.mPixelStep;
                        pImgU += image.mPixelStep;
                        pImgV += image.mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVP :
        {
            switch( image.mFormat )
            {
                case FMT_GREY16 :
                    imgShift -= 8;
                    // Fallthru
                case FMT_GREY :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    unsigned char uv = 128>>imgShift;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (YUVJ2YUV_Y(*pImg)>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + uv;
                        *pRefV = *pRefV - (*pRefV>>refShift) + uv;
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (RGB2YUV_Y(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (RGB2YUV_U(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (RGB2YUV_V(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    imgShift -= 8;
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (RGB2YUV_Y(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (RGB2YUV_U(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (RGB2YUV_V(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (*pImg>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (*(pImg+1)>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (*(pImg+2)>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (YUVJ2YUV_Y(*pImg)>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (YUVJ2YUV_U(*(pImg+1))>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (YUVJ2YUV_V(*(pImg+2))>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    unsigned char *pImgY = pImg;
                    unsigned char *pImgU = pImgY+image.mPlaneSize;
                    unsigned char *pImgV = pImgU+image.mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (YUVJ2YUV_Y(*pImgY)>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (YUVJ2YUV_U(*pImgU)>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (YUVJ2YUV_V(*pImgV)>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImgY += mPixelStep;
                        pImgU += mPixelStep;
                        pImgV += mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVJP :
        {
            switch( image.mFormat )
            {
                case FMT_GREY16 :
                    imgShift -= 8;
                    // Fallthru
                case FMT_GREY :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    unsigned char uv = 128>>imgShift;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (*pImg>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + uv;
                        *pRefV = *pRefV - (*pRefV>>refShift) + uv;
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (RGB2YUVJ_Y(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (RGB2YUVJ_U(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (RGB2YUVJ_V(*pImg,*(pImg+1),*(pImg+2))>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_RGB48 :
                {
                    imgShift -= 8;
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (RGB2YUVJ_Y(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (RGB2YUVJ_U(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (RGB2YUVJ_V(*pImg,*(pImg+2),*(pImg+4))>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUV :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (YUV2YUVJ_Y(*pImg)>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (YUV2YUVJ_U(*(pImg+1))>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (YUV2YUVJ_V(*(pImg+2))>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJ :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (*pImg>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (*(pImg+1)>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (*(pImg+2)>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_YUVP :
                {
                    unsigned char *pRefY = pRef;
                    unsigned char *pRefU = pRefY+mPlaneSize;
                    unsigned char *pRefV = pRefU+mPlaneSize;
                    unsigned char *pImgY = pImg;
                    unsigned char *pImgU = pImgY+image.mPlaneSize;
                    unsigned char *pImgV = pImgU+image.mPlaneSize;
                    while( pRefV < mBuffer.tail() )
                    {
                        *pRefY = *pRefY - (*pRefY>>refShift) + (YUV2YUVJ_Y(*pImgY)>>imgShift);
                        *pRefU = *pRefU - (*pRefU>>refShift) + (YUV2YUVJ_U(*pImgU)>>imgShift);
                        *pRefV = *pRefV - (*pRefV>>refShift) + (YUV2YUVJ_V(*pImgV)>>imgShift);
                        pRefY += mPixelStep;
                        pRefU += mPixelStep;
                        pRefV += mPixelStep;
                        pImgY += mPixelStep;
                        pImgU += mPixelStep;
                        pImgV += mPixelStep;
                    }
                    return;
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pRef = *pRef - (*pRef>>refShift) + (*pImg>>imgShift);
                        pRef += mPixelStep;
                        pImg += image.mPixelStep;
                    }
                    return;
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_UNDEF :
            break;
    }
    Panic( "Unsupported blend of format %d with format %d", mFormat, image.mFormat );
    return;
}

/**
* @brief 
*
* @param fraction
*/
void Image::decay( unsigned short fraction ) const
{
    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to decay image with undefined format" );
    int refShift = 0;
    for ( unsigned int i = 2; i < 1<<16; i *= 2 )
    {
        refShift++;
        if ( i == fraction )
            break;
    }
    if ( refShift == 0 )
        Fatal( "Decay fraction must be power of two, %d not valid", fraction );
    if ( fraction > 16 && mFormat != FMT_GREY16 )
        Warning( "Attempt to decay image of format %d with fraction %d, too high", mFormat, fraction );
    if ( refShift >= mColourDepth )
    {
        Warning( "Reference image shift of %d bits will have no effect, not decaying", refShift );
        return;
    }
    Debug( 5, "Decaying format %d with fraction %d, refshift %d", mFormat, fraction, refShift );
    unsigned char *pRef = mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY16 :
        case FMT_RGB48 :
        {
            while( pRef < mBuffer.tail() )
            {
                unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                ref = ref - (ref>>refShift);
                *pRef = (unsigned char)(ref >> 8);
                *(pRef+1) = (unsigned char)ref;
                pRef += mPixelWidth;
            }
            return;
        }
        case FMT_GREY :
        case FMT_RGB :
        case FMT_YUVJ :
        case FMT_YUVJP :
        {
            while( pRef < mBuffer.tail() )
            {
                *pRef = *pRef - (*pRef>>refShift);
                pRef++;
            }
            return;
        }
        case FMT_YUV :
        case FMT_YUVP :
        {
            while( pRef < mBuffer.tail() )
            {
                *pRef = clamp( *pRef - (*pRef>>refShift), 16, 256 );
                pRef++;
            }
            return;
        }
        case FMT_UNDEF :
            break;
    }
    Panic( "Unsupported decay of format %d", mFormat );
    return;
}

/**
* @brief 
*
* @param fraction
*/
void Image::boost( unsigned short fraction ) const
{
    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to boost image with undefined format" );
    int refShift = 0;
    for ( unsigned int i = 2; i < 1<<16; i *= 2 )
    {
        refShift++;
        if ( i == fraction )
            break;
    }
    if ( refShift == 0 )
        Fatal( "Boost fraction must be power of two, %d not valid", fraction );
    if ( fraction > 16 && mFormat != FMT_GREY16 )
        Warning( "Attempt to boost image of format %d with fraction %d, too high", mFormat, fraction );
    if ( refShift >= mColourDepth )
    {
        Warning( "Image shift of %d bits will have no effect, not boosting", refShift );
        return;
    }
    Debug( 5, "Boosting format %d with fraction %d, refshift %d", mFormat, fraction, refShift );
    unsigned char *pRef = mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY16 :
        case FMT_RGB48 :
        {
            while( pRef < mBuffer.tail() )
            {
                unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                ref = clamp( ref + (ref>>refShift), 0, 65536 );
                *pRef = (unsigned char)(ref >> 8);
                *(pRef+1) = (unsigned char)ref;
                pRef += mPixelWidth;
            }
            return;
        }
        case FMT_GREY :
        case FMT_RGB :
        case FMT_YUVJ :
        case FMT_YUVJP :
        {
            while( pRef < mBuffer.tail() )
            {
                *pRef = clamp( *pRef + (*pRef>>refShift), 0, 256 );
                pRef++;
            }
            return;
        }
        case FMT_YUV :
        {
            while( pRef < mBuffer.tail() )
            {
                *pRef = clamp( *pRef + (*pRef>>refShift), 16, 240 );
                *(pRef+1) = clamp( *(pRef+1) + (*(pRef+1)>>refShift), 16, 235 );
                *(pRef+2) = clamp( *(pRef+2) + (*(pRef+2)>>refShift), 16, 235 );
                pRef += mPixelStep;
            }
            return;
        }
        case FMT_YUVP :
        {
            unsigned char *pRefY = pRef;
            unsigned char *pRefU = pRefY+mPlaneSize;
            unsigned char *pRefV = pRefU+mPlaneSize;
            while( pRefV < mBuffer.tail() )
            {
                *pRefY = clamp( *pRefY + (*pRefY>>refShift), 16, 240 );
                *pRefU = clamp( *pRefU + (*pRefU>>refShift), 16, 235 );
                *pRefV = clamp( *pRefV + (*pRefV>>refShift), 16, 235 );
                pRefY++;
                pRefU++;
                pRefV++;
            }
            return;
        }
        case FMT_UNDEF :
            break;
    }
    Panic( "Unsupported decay of format %d", mFormat );
    return;
}

/**
* @brief 
*/
void Image::square() const
{
    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to square image with undefined format" );

    unsigned char *pRef = mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY16 :
        case FMT_RGB48 :
        {
            while( pRef < mBuffer.tail() )
            {
                unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                unsigned int sq = (ref * ref) >> 16;
                *pRef = (unsigned char)(sq >> 8);
                *(pRef+1) = (unsigned char)sq;
                pRef += mPixelWidth;
            }
            return;
        }
        case FMT_UNDEF :
        default :
            break;
    }
    Panic( "Unsupported square of format %d", mFormat );
    return;
}

/**
* @brief 
*/
void Image::squareRoot() const
{
    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to square root image with undefined format" );

    unsigned char *pRef = mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY16 :
        case FMT_RGB48 :
        {
            while( pRef < mBuffer.tail() )
            {
                unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                unsigned int sq = ref<<16;
                unsigned short rt = (unsigned short)sqrt(sq);
                *pRef = (unsigned char)(rt >> 8);
                *(pRef+1) = (unsigned char)rt;
                pRef += mPixelWidth;
            }
            return;
        }
        case FMT_UNDEF :
        default :
            break;
    }
    Panic( "Unsupported square root of format %d", mFormat );
    return;
}

/**
* @brief 
*
* @return 
*/
unsigned long long Image::total() const
{
    unsigned char *pData = mBuffer.data();
    unsigned long long total = 0;

    switch( mFormat )
    {
        case FMT_GREY :
        {
            while( pData < mBuffer.tail() )
                total += *pData++;
            break;
        }
        case FMT_GREY16 :
        {
            while( pData < mBuffer.tail() )
            {
                total += *pData;
                pData += mPixelStep;
            }
            break;
        }
        case FMT_RGB :
        {
            while( pData < mBuffer.tail() )
            {
                total += RGB2YUVJ_Y(*(pData),*(pData+1),*(pData+2));
                pData += mPixelStep;
            }
            break;
        }
        case FMT_RGB48 :
        {
            while( pData < mBuffer.tail() )
            {
                total += RGB2YUVJ_Y(*(pData),*(pData+2),*(pData+4));
                pData += mPixelStep;
            }
            break;
        }
        case FMT_YUV :
        {
            while( pData < mBuffer.tail() )
            {
                total += YUV2YUVJ_Y(*pData);
                pData += mPixelStep;
            }
            break;
        }
        case FMT_YUVJ :
        {
            while( pData < mBuffer.tail() )
            {
                total += *pData;
                pData += mPixelStep;
            }
            break;
        }
        case FMT_YUVP :
        {
            unsigned char *pDataU = pData+mPlaneSize;
            while( pData < pDataU )
                total += YUV2YUVJ_Y(*pData++);
            break;
        }
        case FMT_YUVJP :
        {
            unsigned char *pDataU = pData+mPlaneSize;
            while( pData < pDataU )
                total += *pData++;
            break;
        }
        case FMT_UNDEF :
            break;
    }
    return( total );
}

/**
* @brief 
*
* @param channel
*
* @return 
*/
unsigned long long Image::total( int channel ) const
{
    if ( channel <= 0 || channel > mChannels )
        Panic( "Unable to total channel %d for format %d", channel, mFormat );

    unsigned char *pData = mBuffer.data();
    long long total = 0;

    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_GREY16 :
        {
            return( this->total() );
        }
        case FMT_RGB :
        case FMT_RGB48 :
        {
            pData += (channel-1)*mPixelWidth;
            while( pData < mBuffer.tail() )
            {
                total += *pData;
                pData += mPixelStep;
            }
            break;
        }
        case FMT_YUV :
        case FMT_YUVJ :
        {
            pData += channel-1;
            if ( channel == 1 )
            {
                while( pData < mBuffer.tail() )
                {
                    total += *pData;
                    pData += mPixelStep;
                }
            }
            else
            {
                while( pData < mBuffer.tail() )
                {
                    total += (signed char)*pData;
                    pData += mPixelStep;
                }
            }
            break;
        }
        case FMT_YUVP :
        case FMT_YUVJP :
        {
            pData += ((channel-1) * mPlaneSize);
            unsigned char *pDataNext = pData+mPlaneSize;
            if ( channel == 1 )
            {
                while( pData < pDataNext )
                    total += *pData++;
            }
            else
            {
                while( pData < pDataNext )
                    total += (signed char)*pData++;
            }
            break;
        }
        case FMT_UNDEF :
            break;
    }
    return( total );
}

/**
* @brief 
*
* @param factor
*/
void Image::shrink( unsigned char factor )
{
    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to shrink image with undefined format" );

    bool factorOk = false;
    for ( unsigned int i = 2; i < 1<<8; i *= 2 )
    {
        if ( i == factor )
        {
            factorOk = true;
            break;
        }
    }
    if ( !factorOk )
        Fatal( "Shrink factor must be power of two, %d not valid", factor );

    int samplePixels = factor * factor;

    Image scaleImage( mFormat, mWidth/factor, mHeight/factor );

    unsigned char *pSrc = mBuffer.data();
    unsigned char *pDst = scaleImage.mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_YUVP :
        case FMT_YUVJP :
        {
            for ( int y = 0; y <= mHeight-factor; y += factor )
            {
                for ( int x = 0; x <= mWidth-factor; x += factor )
                {
                    unsigned long total = 0;
                    for ( int dY = 0; dY < factor; dY++ )
                    {
                        for ( int dX = 0; dX < factor; dX++ )
                        {
                            total += *(pSrc + dX + (mStride * dY));
                        }
                    }
                    pSrc += factor;
                    *pDst++ = total/samplePixels;
                }
                pSrc += (factor-1)*mStride;
            }
            assign( scaleImage );
            return;
        }
        case FMT_RGB :
        case FMT_YUV :
        case FMT_YUVJ :
        {
            for ( int y = 0; y <= mHeight-factor; y += factor )
            {
                for ( int x = 0; x <= mWidth-factor; x += factor )
                {
                    unsigned long total1 = 0;
                    unsigned long total2 = 0;
                    unsigned long total3 = 0;
                    for ( int dY = 0; dY < factor; dY++ )
                    {
                        for ( int dX = 0; dX < factor; dX++ )
                        {
                            total1 += *(pSrc + dX + (mStride * dY));
                            total2 += *(pSrc + 1 + dX + (mStride * dY));
                            total3 += *(pSrc + 2 + dX + (mStride * dY));
                        }
                    }
                    pSrc += mPixelStep * factor;
                    *pDst++ = total1/samplePixels;
                    *pDst++ = total2/samplePixels;
                    *pDst++ = total3/samplePixels;
                }
                pSrc += (factor-1)*mStride;
            }
            assign( scaleImage );
            return;
        }
        case FMT_RGB48 :
        {
            for ( int y = 0; y <= mHeight-factor; y += factor )
            {
                for ( int x = 0; x <= mWidth-factor; x += factor )
                {
                    unsigned long total1 = 0;
                    unsigned long total2 = 0;
                    unsigned long total3 = 0;
                    for ( int dY = 0; dY < factor; dY++ )
                    {
                        for ( int dX = 0; dX < factor; dX++ )
                        {
                            total1 += ((unsigned short)*(pSrc + dX + (mStride * dY)))<<8;
                            total1 += *(pSrc + 1 + dX + (mStride * dY));
                            total2 += ((unsigned short)*(pSrc + 2 + dX + (mStride * dY)))<<8;
                            total2 += *(pSrc + 3 + dX + (mStride * dY));
                            total3 += ((unsigned short)*(pSrc + 4 + dX + (mStride * dY)))<<8;
                            total3 += *(pSrc + 5 + dX + (mStride * dY));
                        }
                    }
                    pSrc += mPixelStep * factor;
                    *pDst++ = (total1/samplePixels)>>8;
                    *pDst++ = (total1/samplePixels);
                    *pDst++ = (total2/samplePixels)>>8;
                    *pDst++ = (total2/samplePixels);
                    *pDst++ = (total3/samplePixels)>>8;
                    *pDst++ = (total3/samplePixels);
                }
                pSrc += (factor-1)*mStride;
            }
            assign( scaleImage );
            return;
        }
        case FMT_GREY16 :
        {
            for ( int y = 0; y <= mHeight-factor; y += factor )
            {
                for ( int x = 0; x <= mWidth-factor; x += factor )
                {
                    unsigned long total = 0;
                    for ( int dY = 0; dY < factor; dY++ )
                    {
                        for ( int dX = 0; dX < factor; dX++ )
                        {
                            total += ((unsigned short)*(pSrc + (mPixelStep*(dX + (mWidth * dY)))))<<8;
                            total += *(pSrc + 1 + (mPixelStep*(dX + (mWidth * dY))));
                        }
                    }
                    pSrc += factor*mPixelStep;
                    unsigned short newValue = total/samplePixels;
                    *pDst = newValue>>8;
                    *(pDst+1) = newValue;
                    pDst += scaleImage.mPixelStep;
                }
                pSrc += (factor-1)*mStride;
            }
            assign( scaleImage );
            return;
        }
        case FMT_UNDEF :
        default :
            break;
    }
    Panic( "Unsupported shrink of format %d", mFormat );
    return;
}

/**
* @brief 
*
* @param factor
*/
void Image::swell( unsigned char factor )
{
    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to swell image with undefined format" );

    bool factorOk = false;
    for ( unsigned int i = 2; i < 1<<8; i *= 2 )
    {
        if ( i == factor )
        {
            factorOk = true;
            break;
        }
    }
    if ( !factorOk )
        Fatal( "Swell factor must be power of two, %d not valid", factor );

    Image scaleImage( mFormat, mWidth*factor, mHeight*factor );

    unsigned char *pSrc = mBuffer.data();
    unsigned char *pDst = scaleImage.mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_YUVP :
        case FMT_YUVJP :
        {
            for ( int y = 0; y < mHeight; y++ )
            {
                for ( int x = 0; x < mWidth; x++ )
                {
                    for ( int dX = 0; dX < factor; dX++ )
                    {
                        *pDst = *pSrc;
                        pDst++;
                    }
                    pSrc++;
                }
                memcpy( pDst, pDst-scaleImage.mStride, scaleImage.mStride );
                pDst += scaleImage.mStride;
            }
            assign( scaleImage );
            return;
        }
        case FMT_RGB :
        case FMT_YUV :
        case FMT_YUVJ :
        {
            for ( int y = 0; y < mHeight; y++ )
            {
                for ( int x = 0; x < mWidth; x++ )
                {
                    for ( int dX = 0; dX < factor; dX++ )
                    {
                        *pDst = *pSrc;
                        *(pDst+1) = *(pSrc+1);
                        *(pDst+2) = *(pSrc+2);
                        pDst += mPixelStep;
                    }
                    pSrc += mPixelStep;
                }
                memcpy( pDst, pDst-scaleImage.mStride, scaleImage.mStride );
                pDst += scaleImage.mStride;
            }
            assign( scaleImage );
            return;
        }
        case FMT_RGB48 :
        {
            for ( int y = 0; y < mHeight; y++ )
            {
                for ( int x = 0; x < mWidth; x++ )
                {
                    for ( int dX = 0; dX < factor; dX++ )
                    {
                        memcpy( pDst, pSrc, mPixelStep );
                        pDst += mPixelStep;
                    }
                    pSrc += mPixelStep;
                }
                memcpy( pDst, pDst-scaleImage.mStride, scaleImage.mStride );
                pDst += scaleImage.mStride;
            }
            assign( scaleImage );
            return;
        }
        case FMT_GREY16 :
        {
            for ( int y = 0; y < mHeight; y++ )
            {
                for ( int x = 0; x < mWidth; x++ )
                {
                    for ( int dX = 0; dX < factor; dX++ )
                    {
                        *pDst = *pSrc;
                        *(pDst+1) = *(pSrc+1);
                        pDst += scaleImage.mPixelStep;
                    }
                    pSrc += mPixelStep;
                }
                memcpy( pDst, pDst-scaleImage.mStride, scaleImage.mStride );
                pDst += scaleImage.mStride;
            }
            assign( scaleImage );
            return;
        }
        case FMT_UNDEF :
        default :
            break;
    }
    Panic( "Unsupported swell of format %d", mFormat );
    return;
}

/**
* @brief 
*
* @param filterSize
*/
void Image::despeckleFilter( int filterSize )
{
    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to despeckle filter image with undefined format" );

    int filtW = filterSize;
    int filtH = filterSize;
    if ( filtW < 1 && filtH < 1 )
        return;

    int filtMaxX = filtW - 1;
    int filtMaxY = filtH - 1;

    unsigned char *pRef = mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY :
        {
            int loBlockX, hiBlockX, loBlockY, hiBlockY;
            bool inBlock;
            unsigned char *pTempRef;
            int loY = 0;
            int hiY = mHeight-1;
            for ( int y = loY; y <= hiY; y++ )
            {
                int loX = 0;
                int hiX = mWidth-1;

                for ( int x = loX; x <= hiX; x++, pRef++ )
                {
                    if ( *pRef )
                    {
                        loBlockX = (x>=(loX+filtMaxX))?-filtMaxX:loX-x;
                        hiBlockX = (x<=(hiX-filtMaxX))?0:((hiX-x)-filtMaxX);
                        loBlockY = (y>=(loY+filtMaxY))?-filtMaxY:loY-y;
                        hiBlockY = (y<=(hiY-filtMaxY))?0:((hiY-y)-filtMaxY);
                        inBlock = false;
                        for ( int dY = loBlockY; !inBlock && dY <= hiBlockY; dY++ )
                        {
                            for ( int dX = loBlockX; !inBlock && dX <= hiBlockX; dX++ )
                            {
                                inBlock = true;
                                for ( int dY2 = 0; inBlock && dY2 < filtH; dY2++ )
                                {
                                    for ( int dX2 = 0; inBlock && dX2 < filtW; dX2++ )
                                    {
                                        pTempRef = pRef + dX + dX2 + (mStride * (dY + dY2));
                                        if ( !*pTempRef )
                                            inBlock = false;
                                    }
                                }
                            }
                        }
                        if ( !inBlock )
                            *pRef = BLACK;
                    }
                }
            }
            return;
        }
        case FMT_GREY16 :
        {
            int loBlockX, hiBlockX, loBlockY, hiBlockY;
            bool inBlock;
            unsigned char *pTempRef;
            int loY = 0;
            int hiY = mHeight-1;
            for ( int y = loY; y <= hiY; y++ )
            {
                int loX = 0;
                int hiX = mWidth-1;

                for ( int x = loX; x <= hiX; x++, pRef += mPixelStep )
                {
                    if ( *pRef || *(pRef+1) )
                    {
                        loBlockX = (x>=(loX+filtMaxX))?-filtMaxX:loX-x;
                        hiBlockX = (x<=(hiX-filtMaxX))?0:((hiX-x)-filtMaxX);
                        loBlockY = (y>=(loY+filtMaxY))?-filtMaxY:loY-y;
                        hiBlockY = (y<=(hiY-filtMaxY))?0:((hiY-y)-filtMaxY);
                        inBlock = false;
                        for ( int dY = loBlockY; !inBlock && dY <= hiBlockY; dY++ )
                        {
                            for ( int dX = loBlockX; !inBlock && dX <= hiBlockX; dX++ )
                            {
                                inBlock = true;
                                for ( int dY2 = 0; inBlock && dY2 < filtH; dY2++ )
                                {
                                    for ( int dX2 = 0; inBlock && dX2 < filtW; dX2++ )
                                    {
                                        pTempRef = pRef + (mPixelStep * (dX + dX2 + (mWidth * (dY + dY2))));
                                        if ( !(*pTempRef || *(pTempRef+1)) )
                                            inBlock = false;
                                    }
                                }
                            }
                        }
                        if ( !inBlock )
                        {
                            *pRef = 0;
                            *(pRef+1) = 0;
                        }
                    }
                }
            }
            return;
        }
        case FMT_UNDEF :
        default :
            break;
    }
    Panic( "Unsupported despeckle filter of format %d", mFormat );
    return;
}

/**
* @brief 
*
* @param blobGroup
* @param mask
* @param weightBlobCentres
* @param tidyBlobs
*
* @return 
*/
int Image::locateBlobs( BlobGroup &blobGroup, const Mask *mask, bool weightBlobCentres, bool tidyBlobs )
{
    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to locate blobs in image with undefined format" );

    if ( mask->hiX() >= mWidth || mask->hiY() >= mHeight )
        Panic( "Mask larger than image" );

    unsigned char *pRef = mBuffer.data();

    if ( !mask )
        mask = new BoxMask( Box( mWidth, mHeight ) );
    const Image *maskImage = mask->image();
    const ByteBuffer &maskBuffer = mask->image()->buffer();
    const unsigned char *pMask = maskBuffer.data();

    int maskLoY = mask->loY();
    int maskHiY = mask->hiY();
    int maskLoX = mask->loX();
    int maskHiX = mask->hiX();

    int blobCount = 0;
    BlobData blobs[256];
    memset( blobs, 0, sizeof(blobs) );

    switch( mFormat )
    {
        case FMT_GREY :
        {
            int lastX, lastY;
            for ( int y = maskLoY, maskY = 0; y <= maskHiY; y++, maskY++ )
            {
                int maskLineLoX = mask->loX(maskY);
                int maskLineHiX = mask->hiX(maskY);
                pRef = mBuffer.data() + (maskLineLoX+(y*mWidth))*mPixelStep;
                pMask = maskBuffer.data() + (maskLineLoX-maskLoX) + ((y-maskLoY)*maskImage->mWidth);
                for ( int x = maskLineLoX; x <= maskLineHiX; x++, pRef++, pMask++ )
                {
                    if ( *pMask && *pRef == WHITE )
                    {
                        lastX = (x>maskLineLoX&&*(pMask-1))?*(pRef-1):0;
                        lastY = (y>maskLoY&&*(pMask-maskImage->mStride))?*(pRef-mStride):0;

                        if ( lastX )
                        {
                            BlobData *xBlob = &blobs[lastX];
                            if ( lastY )
                            {
                                BlobData *yBlob = &blobs[lastY];
                                if ( lastX == lastY )
                                {
                                    // Add to the blob from the x side (either side really)
                                    *pRef = lastX;
                                    xBlob->pixels++;
                                    if ( x > xBlob->hiX ) xBlob->hiX = x;
                                    if ( y > xBlob->hiY ) xBlob->hiY = y;
                                }
                                else
                                {
                                    // Aggregate blobs
                                    BlobData *priBlob = xBlob->pixels>=yBlob->pixels?xBlob:yBlob;
                                    BlobData *secBlob = priBlob==xBlob?yBlob:xBlob;

                                    // Now change all those pixels to the other setting
                                    for ( int blobY = secBlob->loY, blobOffY = secBlob->loY-maskLoY; blobY <= secBlob->hiY; blobY++, blobOffY++ )
                                    {
                                        int loBlobX = secBlob->loX>=mask->loX(blobOffY)?secBlob->loX:mask->loX(blobOffY);
                                        int hiBlobX = secBlob->hiX<=mask->hiX(blobOffY)?secBlob->hiX:mask->hiX(blobOffY);

                                        unsigned char *pBlob = mBuffer.data() + ((loBlobX+(blobY*mWidth))*mPixelStep);
                                        const unsigned char *pTempMask = maskBuffer.data() + (loBlobX-maskLoX) + ((blobY-maskLoY)*maskImage->mWidth);
                                        for ( int blobX = loBlobX; blobX <= hiBlobX; blobX++, pBlob++, pTempMask++ )
                                            if ( *pTempMask && (*pBlob == secBlob->tag) )
                                                *pBlob = priBlob->tag;
                                    }
                                    *pRef = priBlob->tag;

                                    // Merge the slave blob into the master
                                    priBlob->pixels += secBlob->pixels+1;
                                    if ( x > priBlob->hiX ) priBlob->hiX = x;
                                    if ( y > priBlob->hiY ) priBlob->hiY = y;
                                    if ( secBlob->loX < priBlob->loX ) priBlob->loX = secBlob->loX;
                                    if ( secBlob->loY < priBlob->loY ) priBlob->loY = secBlob->loY;
                                    if ( secBlob->hiX > priBlob->hiX ) priBlob->hiX = secBlob->hiX;
                                    if ( secBlob->hiY > priBlob->hiY ) priBlob->hiY = secBlob->hiY;

                                    memset( secBlob, 0, sizeof(*secBlob) );
                                    blobCount--;
                                }
                            }
                            else
                            {
                                // Add to the blob from the x side 
                                *pRef = lastX;
                                xBlob->pixels++;
                                if ( x > xBlob->hiX ) xBlob->hiX = x;
                                if ( y > xBlob->hiY ) xBlob->hiY = y;
                            }
                        }
                        else
                        {
                            if ( lastY )
                            {
                                // Add to the blob from the y side
                                BlobData *yBlob = &blobs[lastY];

                                *pRef = lastY;
                                yBlob->pixels++;
                                if ( x > yBlob->hiX ) yBlob->hiX = x;
                                if ( y > yBlob->hiY ) yBlob->hiY = y;
                            }
                            else
                            {
                                // Create a new blob
                                int i;
                                for ( i = (WHITE-1); i > 0; i-- )
                                {
                                    BlobData *blob = &blobs[i];
                                    // See if we can recycle one first, only if it's at least two rows up
                                    if ( blob->pixels && blob->hiY < (y-1) )
                                    {
                                        if ( (blobGroup.mMinSize && blob->pixels < blobGroup.mMinSize) || (blobGroup.mMaxSize && blob->pixels > blobGroup.mMaxSize) )
                                        {
                                            if ( tidyBlobs )
                                            {
                                                for ( int blobY = blob->loY; blobY <= blob->hiY; blobY++ )
                                                {
                                                    unsigned char *pBlob = mBuffer.data() + (blob->loX+(blobY*mWidth))*mPixelStep;
                                                    const unsigned char *pTempMask = maskBuffer.data() + (blob->loX-maskLoX) + ((blobY-maskLoY)*maskImage->mWidth);
                                                    for ( int blobX = blob->loX; blobX <= blob->hiX; blobX++, pBlob++, pTempMask++ )
                                                        if ( *pTempMask && *pBlob == blob->tag )
                                                            *pBlob = BLACK;
                                                }
                                            }
                                            memset( blob, 0, sizeof(*blob) );
                                            blobCount--;
                                        }
                                    }
                                    if ( !blob->pixels )
                                    {
                                        *pRef = i;
                                        blob->tag = i;
                                        blob->pixels++;
                                        blob->loX = blob->hiX = x;
                                        blob->loY = blob->hiY = y;
                                        blobCount++;
                                        break;
                                    }
                                }
                                if ( i == 0 )
                                {
                                    Warning( "Max blob count reached. Unable to allocate new blobs so terminating. Zone settings may be too sensitive." );
                                    x = maskHiX+1;
                                    y = maskHiY+1;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        case FMT_GREY16 :
        {
            int lastX, lastY;
            for ( int y = maskLoY, maskY = 0; y <= maskHiY; y++, maskY++ )
            {
                int maskLineLoX = mask->loX(maskY);
                int maskLineHiX = mask->hiX(maskY);
                //printf( "mllX: %d, mlhX: %d\n", maskLineLoX, maskLineHiX );
                pRef = mBuffer.data() + (maskLineLoX+(y*mWidth))*mPixelStep;
                pMask = maskBuffer.data() + (maskLineLoX-maskLoX) + ((y-maskLoY)*maskImage->mWidth);
                //printf( "%d: mI: %p\n", maskY, pMask );
                for ( int x = maskLineLoX; x <= maskLineHiX; x++, pRef += mPixelStep, pMask++ )
                {
                    //if ( *pMask && (*pRef == WHITE || *(pRef+1) == WHITE) )
                    if ( *pMask && *pRef == WHITE )
                    {
                        lastX = (x>maskLineLoX&&*(pMask-1))?*(pRef-mPixelStep):0;
                        lastY = (y>maskLoY&&*(pMask-maskImage->mStride))?*(pRef-mStride):0;

                        //printf( "At %d x %d\n", x, y );
                        //printf( "mI-1: %p, mI-%d: %p\n", pMask-1, maskImage->mStride, pMask-maskImage->mStride );
                        if ( lastX )
                        {
                            BlobData *xBlob = &blobs[lastX];
                            if ( lastY )
                            {
                                BlobData *yBlob = &blobs[lastY];
                                if ( lastX == lastY )
                                {
                                    // Add to the blob from the x side (either side really)
                                    *pRef = lastX;
                                    *(pRef+1) = 0;
                                    xBlob->pixels++;
                                    if ( x > xBlob->hiX ) xBlob->hiX = x;
                                    if ( y > xBlob->hiY ) xBlob->hiY = y;
                                }
                                else
                                {
                                    //printf( "lx: %d != ly:%d\n", lastX, lastY );
                                    // Aggregate blobs
                                    BlobData *priBlob = xBlob->pixels>=yBlob->pixels?xBlob:yBlob;
                                    BlobData *secBlob = priBlob==xBlob?yBlob:xBlob;

                                    // Now change all those pixels to the other setting
                                    for ( int blobY = secBlob->loY, blobOffY = secBlob->loY-maskLoY; blobY <= secBlob->hiY; blobY++, blobOffY++ )
                                    {
                                        int loBlobX = secBlob->loX>=mask->loX(blobOffY)?secBlob->loX:mask->loX(blobOffY);
                                        int hiBlobX = secBlob->hiX<=mask->hiX(blobOffY)?secBlob->hiX:mask->hiX(blobOffY);

                                        //printf( "Clearing %d from %d x %d -> %d x %d\n", secBlob->tag, secBlob->loY, loBlobX, secBlob->hiY, hiBlobX );
                                        unsigned char *pBlob = mBuffer.data() + ((loBlobX+(blobY*mWidth))*mPixelStep);
                                        const unsigned char *pTempMask = maskBuffer.data() + (loBlobX-maskLoX) + ((blobY-maskLoY)*maskImage->mWidth);
                                        for ( int blobX = loBlobX; blobX <= hiBlobX; blobX++, pBlob += mPixelStep, pTempMask++ )
                                            if ( *pTempMask && (*pBlob == secBlob->tag) )
                                            {
                                                *pBlob = priBlob->tag;
                                                *(pBlob+1) = 0;
                                            }
                                    }
                                    *pRef = priBlob->tag;
                                    *(pRef+1) = 0;

                                    // Merge the slave blob into the master
                                    priBlob->pixels += secBlob->pixels+1;
                                    if ( x > priBlob->hiX ) priBlob->hiX = x;
                                    if ( y > priBlob->hiY ) priBlob->hiY = y;
                                    if ( secBlob->loX < priBlob->loX ) priBlob->loX = secBlob->loX;
                                    if ( secBlob->loY < priBlob->loY ) priBlob->loY = secBlob->loY;
                                    if ( secBlob->hiX > priBlob->hiX ) priBlob->hiX = secBlob->hiX;
                                    if ( secBlob->hiY > priBlob->hiY ) priBlob->hiY = secBlob->hiY;

                                    //printf( "Merging %d to %d\n", secBlob->tag, priBlob->tag );
                                    memset( secBlob, 0, sizeof(*secBlob) );
                                    blobCount--;
                                }
                            }
                            else
                            {
                                // Add to the blob from the x side 
                                *pRef = lastX;
                                *(pRef+1) = 0;
                                xBlob->pixels++;
                                if ( x > xBlob->hiX ) xBlob->hiX = x;
                                if ( y > xBlob->hiY ) xBlob->hiY = y;
                            }
                        }
                        else
                        {
                            if ( lastY )
                            {
                                // Add to the blob from the y side
                                BlobData *yBlob = &blobs[lastY];

                                *pRef = lastY;
                                *(pRef+1) = 0;
                                yBlob->pixels++;
                                if ( x > yBlob->hiX ) yBlob->hiX = x;
                                if ( y > yBlob->hiY ) yBlob->hiY = y;
                            }
                            else
                            {
                                // Create a new blob
                                int i;
                                for ( i = (WHITE-1); i > 0; i-- )
                                {
                                    BlobData *blob = &blobs[i];
                                    // See if we can recycle one first, only if it's at least two rows up
                                    if ( blob->pixels && blob->hiY < (y-1) )
                                    {
                                        if ( (blobGroup.mMinSize && blob->pixels < blobGroup.mMinSize) || (blobGroup.mMaxSize && blob->pixels > blobGroup.mMaxSize) )
                                        {
                                            if ( tidyBlobs )
                                            {
                                                for ( int blobY = blob->loY; blobY <= blob->hiY; blobY++ )
                                                {
                                                    unsigned char *pBlob = mBuffer.data() + (blob->loX+(blobY*mWidth))*mPixelStep;
                                                    //const unsigned char *pTempMask = maskBuffer.data() + ((blobY-maskLoY)*maskImage->mWidth);
                                                    const unsigned char *pTempMask = maskBuffer.data() + (blob->loX-maskLoX) + ((blobY-maskLoY)*maskImage->mWidth);
                                                    for ( int blobX = blob->loX; blobX <= blob->hiX; blobX++, pBlob += mPixelStep, pTempMask++ )
                                                        if ( *pTempMask && *pBlob == blob->tag )
                                                        {
                                                            *pBlob = BLACK;
                                                            *(pBlob+1) = BLACK;
                                                        }
                                                }
                                            }
                                            //printf( "Recycling %d\n", blob->tag );
                                            memset( blob, 0, sizeof(*blob) );
                                            blobCount--;
                                        }
                                    }
                                    if ( !blob->pixels )
                                    {
                                        *pRef = i;
                                        *(pRef+1) = 0;
                                        blob->tag = i;
                                        blob->pixels++;
                                        blob->loX = blob->hiX = x;
                                        blob->loY = blob->hiY = y;
                                        //printf( "Creating %d\n", blob->tag );
                                        blobCount++;
                                        break;
                                    }
                                }
                                if ( i == 0 )
                                {
                                    Warning( "Max blob count reached. Unable to allocate new blobs so terminating. Zone settings may be too sensitive." );
                                    x = maskHiX+1;
                                    y = maskHiY+1;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
        case FMT_UNDEF :
        default :
            Panic( "Unsupported blob location in image format %d", mFormat );
            break;
    }
    if ( blobCount )
    {
        int minBlobSize = 0;
        int maxBlobSize = 0;
        int blobLoX = 0;
        int blobHiX = 0;
        int blobLoY = 0;
        int blobHiY = 0;
        unsigned long blobsTotalX = 0;
        unsigned long blobsTotalY = 0;
        // Now eliminate blobs under the threshold
        for ( int i = 1; i < WHITE; i++ )
        {
            BlobData *blob = &blobs[i];
            if ( blob->pixels )
            {
                if ( (blobGroup.mMinSize && blob->pixels < blobGroup.mMinSize) || (blobGroup.mMaxSize && blob->pixels > blobGroup.mMaxSize) )
                {
                    // Remove blob
                    if ( tidyBlobs )
                    {
                        for ( int blobY = blob->loY; blobY <= blob->hiY; blobY++ )
                        {
                            unsigned char *pBlob = mBuffer.data() + (blob->loX+(blobY*mWidth))*mPixelStep;
                            const unsigned char *pMask = maskBuffer.data() + (blob->loX-maskLoX) + ((blobY-maskLoY)*maskImage->mWidth);
                            if ( mPixelStep == 1 )
                            {
                                for ( int blobX = blob->loX; blobX <= blob->hiX; blobX++, pBlob++, pMask++ )
                                    if ( *pMask && *pBlob == blob->tag )
                                        *pBlob = BLACK;
                            }
                            else
                            {
                                for ( int blobX = blob->loX; blobX <= blob->hiX; blobX++, pBlob += mPixelStep, pMask++ )
                                    if ( *pMask && *pBlob == blob->tag )
                                    {
                                        *pBlob = BLACK;
                                        *(pBlob+1) = 0;
                                    }
                            }
                        }
                    }
                    //printf( "Removing %d\n", blob->tag );
                    memset( blob, 0, sizeof(*blob) );
                    blobCount--;
                }
                else
                {
                    // Keep blob
                    if ( !minBlobSize || blob->pixels < minBlobSize ) minBlobSize = blob->pixels;
                    if ( !maxBlobSize || blob->pixels > maxBlobSize ) maxBlobSize = blob->pixels;
                    if ( blob->loX < blobLoX ) blobLoX = blob->loX;
                    if ( blob->loY < blobLoY ) blobLoY = blob->loY;
                    if ( blob->hiX < blobHiX ) blobHiX = blob->hiX;
                    if ( blob->hiY < blobHiY ) blobHiY = blob->hiY;
                    blobGroup.mPixels += blob->pixels;
                    if ( weightBlobCentres )
                    {
                        unsigned long totalX = 0;
                        unsigned long totalY = 0;
                        for ( int blobY = blob->loY; blobY <= blob->hiY; blobY++ )
                        {
                            unsigned char *pBlob = mBuffer.data() + (blob->loX+(blobY*mWidth))*mPixelStep;
                            const unsigned char *pMask = maskBuffer.data() + (blob->loX-maskLoX) + ((blobY-maskLoY)*maskImage->mWidth);
                            for ( int blobX = blob->loX; blobX <= blob->hiX; blobX++, pBlob += mPixelStep, pMask++ )
                            {
                                if ( *pMask && *pBlob == blob->tag )
                                {
                                    totalX += blobX;
                                    totalY += blobY;
                                }
                            }
                        }
                        blobsTotalX += totalX;
                        blobsTotalY += totalY;
                        blob->midX = int(round(totalX/blob->pixels));
                        blob->midY = int(round(totalY/blob->pixels));
                    }
                    else
                    {
                        blob->midX = int((blob->hiX+blob->loX)/2);
                        blob->midY = int((blob->hiY+blob->loY)/2);
                    }
                    //printf( "Keeping %d\n", blob->tag );
                    Image blobImage( FMT_GREY, (blob->hiX-blob->loX)+1, (blob->hiY-blob->loY)+1 );
                    for ( int blobY = blob->loY; blobY <= blob->hiY; blobY++ )
                    {
                        unsigned char *pImg = blobImage.buffer().data() + ((blobY-blob->loY)*blobImage.width());
                        unsigned char *pBlob = mBuffer.data() + (blob->loX+(blobY*mWidth))*mPixelStep;
                        const unsigned char *pMask = maskBuffer.data() + (blob->loX-maskLoX) + ((blobY-maskLoY)*maskImage->mWidth);
                        for ( int blobX = blob->loX; blobX <= blob->hiX; blobX++, pBlob += mPixelStep, pMask++, pImg++ )
                            if ( *pMask && *pBlob == blob->tag )
                                *pImg = WHITE;
                    }
                    Blob *blob2 = new Blob( blobImage, Coord( blob->loX, blob->loY ), Coord( blob->midX, blob->midY ), blob->pixels );
                    blobGroup.mBlobList.push_back( blob2 );
                }
            }
        }
        if ( blobCount )
        {
            blobGroup.mMinSize = minBlobSize;
            blobGroup.mMaxSize = maxBlobSize;
            blobGroup.mExtent = Box( blobLoX, blobLoY, blobHiX, blobHiY );
            if ( weightBlobCentres )
                blobGroup.mCentre = Coord( int(round(blobsTotalX/blobGroup.mPixels)), int(round(blobsTotalY/blobGroup.mPixels)) );
            else
                blobGroup.mCentre = blobGroup.mExtent.centre();
        }
    }
    return( blobCount );
}

#if 0
void Image::medianFilter( const Coord &filterSize )
{
    typedef multiset<unsigned short> PixelSet;

    if ( mFormat == FMT_UNDEF )
        Panic( "Attempt to median filter image with undefined format" );

    if ( filterSize.x() < 1 && filterSize.y() < 1 )
        return;

    PixelSet pixelSet( filterSize.x() * filterSize.y() );

    int filtMaxX = (filterSize.x()-1)/2;
    int filtMaxY = (filterSize.y()-1)/2;

    unsigned char *pRef = mBuffer.data();
    ByteBuffer filterBuffer = ByteBuffer( mBuffer.size() );
    unsigned char *pFilt = filterBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY :
        {
            int loBlockX, hiBlockX, loBlockY, hiBlockY;
            unsigned char *pTempRef;
            int loY = 0;
            int hiY = mHeight-1;
            for ( int y = loY; y <= hiY; y++ )
            {
                int loX = 0;
                int hiX = mWidth-1;
                for ( int x = loX; x <= hiX; x++, pRef++ )
                {
                    loBlockX = (x>=(loX+filtMaxX))?-filtMaxX:loX-x;
                    hiBlockX = (x<=(hiX-filtMaxX))?0:((hiX-x)-filtMaxX);
                    loBlockY = (y>=(loY+filtMaxY))?-filtMaxY:loY-y;
                    hiBlockY = (y<=(hiY-filtMaxY))?0:((hiY-y)-filtMaxY);
                    for ( int dY = loBlockY; dY <= hiBlockY; dY++ )
                    {
                        for ( int dX = loBlockX; dX <= hiBlockX; dX++ )
                        {
                            pTempRef = pRef + dX + (mStride * dY);
                            pixelSet.insert( *pTempRef );
                        }
                    }
                    for ( int dY = loBlockY; dY <= hiBlockY; dY++ )
                    {
                        for ( int dX = loBlockX; dX <= hiBlockX; dX++ )
                        {
                            pTempRef = pRef + dX + (mStride * dY);
                            pixelSet.insert( *pTempRef );
                        }
                    }
                }
            }
            return;
        }
        case FMT_GREY16 :
        {
            int loBlockX, hiBlockX, loBlockY, hiBlockY;
            bool inBlock;
            unsigned char *pTempRef;
            int loY = 0;
            int hiY = mHeight-1;
            for ( int y = loY; y <= hiY; y++ )
            {
                int loX = 0;
                int hiX = mWidth-1;

                for ( int x = loX; x <= hiX; x++, pRef += mPixelStep )
                {
                    if ( *pRef || *(pRef+1) )
                    {
                        loBlockX = (x>=(loX+filtMaxX))?-filtMaxX:loX-x;
                        hiBlockX = (x<=(hiX-filtMaxX))?0:((hiX-x)-filtMaxX);
                        loBlockY = (y>=(loY+filtMaxY))?-filtMaxY:loY-y;
                        hiBlockY = (y<=(hiY-filtMaxY))?0:((hiY-y)-filtMaxY);
                        inBlock = false;
                        for ( int dY = loBlockY; !inBlock && dY <= hiBlockY; dY++ )
                        {
                            for ( int dX = loBlockX; !inBlock && dX <= hiBlockX; dX++ )
                            {
                                inBlock = true;
                                for ( int dY2 = 0; inBlock && dY2 < filtH; dY2++ )
                                {
                                    for ( int dX2 = 0; inBlock && dX2 < filtW; dX2++ )
                                    {
                                        pTempRef = pRef + (mPixelStep * (dX + dX2 + (mWidth * (dY + dY2))));
                                        if ( !(*pTempRef || *(pTempRef+1)) )
                                            inBlock = false;
                                    }
                                }
                            }
                        }
                        if ( !inBlock )
                        {
                            *pRef = 0;
                            *(pRef+1) = 0;
                        }
                    }
                }
            }
            return;
        }
        case FMT_UNDEF :
        default :
            break;
    }
    Panic( "Unsupported despeckle filter of format %d", mFormat );
    return;
}
#endif

/**
* @brief 
*
* @param image
* @param correct
*
* @return 
*/
Image *Image::delta( const Image &image, bool correct ) const
{
    if ( !(mWidth == image.mWidth && mHeight == image.mHeight) )
        Panic( "Attempt to get delta of different sized images, expected %dx%d, got %dx%d", mWidth, mHeight, image.mWidth, image.mHeight );
    if ( mFormat == FMT_UNDEF || image.mFormat == FMT_UNDEF )
        Panic( "Attempt to get delta of image with undefined format" );

    if ( mColourSpace != CS_RGB || image.mChannels == 1 )
        return( yDelta( image, correct ) );

    Image *result = 0;
    if ( mFormat == FMT_RGB48 )
        result = new Image( FMT_GREY16, mWidth, mHeight );
    else
        result = new Image( FMT_GREY, mWidth, mHeight );

    Debug( 5, "Diffing format %d with format %d to format %d", mFormat, image.mFormat, result->mFormat );
    unsigned char *pRef = mBuffer.data();
    unsigned char *pCmp = image.mBuffer.data();
    unsigned char *pDiff = result->mBuffer.data();

    if ( image.mColourSpace != CS_RGB )
    {
        Warning( "Correction disabled when comparing format %d with format %d", mFormat, image.mFormat );
        correct = false;
    }

    int corrs[3] = { 0, 0, 0 };
    if ( correct )
    {
        for ( int i = 1; i <= mChannels; i++ )
        {
            unsigned long long refTotal = total( i );
            unsigned long long cmpTotal = image.total( i );
            corrs[i-1] = ::llabs((long long)(cmpTotal - refTotal)) / mPixels;
            Debug( 5, "Applying channel %d correction of %d (%lld-%lld=%lld/%d)", i, corrs[i-1], refTotal, cmpTotal, refTotal-cmpTotal, mPixels );
        }
    }

    switch( mFormat )
    {
        case FMT_RGB :
        {
            switch( image.mFormat )
            {
                case FMT_RGB :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(*pCmp++-*pRef++-corrs[0]);
                        diffG = abs(*pCmp++-*pRef++-corrs[1]);
                        diffB = abs(*pCmp++-*pRef++-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(*pCmp-*pRef++-corrs[0]);
                        diffG = abs(*(pCmp+2)-*pRef++-corrs[1]);
                        diffB = abs(*(pCmp+4)-*pRef++-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                        pCmp += image.mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUV2RGB_R(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++)-corrs[0]);
                        diffG = abs(YUV2RGB_G(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++)-corrs[1]);
                        diffB = abs(YUV2RGB_B(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                        pCmp += image.mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUVJ2RGB_R(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++)-corrs[0]);
                        diffG = abs(YUVJ2RGB_G(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++)-corrs[1]);
                        diffB = abs(YUVJ2RGB_B(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                        pCmp += image.mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    unsigned char *pCmpY = pCmp;
                    unsigned char *pCmpU = pCmpY+(image.mStride*image.mHeight);
                    unsigned char *pCmpV = pCmpU+(image.mStride*image.mHeight);
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUV2RGB_R(*pCmpY,*pCmpU,*pCmpV)-*(pRef++)-corrs[0]);
                        diffG = abs(YUV2RGB_G(*pCmpY,*pCmpU,*pCmpV)-*(pRef++)-corrs[1]);
                        diffB = abs(YUV2RGB_B(*(pCmpY++),*(pCmpU++),*(pCmpV++))-*(pRef++)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    unsigned char *pCmpY = pCmp;
                    unsigned char *pCmpU = pCmpY+(image.mStride*image.mHeight);
                    unsigned char *pCmpV = pCmpU+(image.mStride*image.mHeight);
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUVJ2RGB_R(*pCmpY,*pCmpU,*pCmpV)-*(pRef++)-corrs[0]);
                        diffG = abs(YUVJ2RGB_G(*pCmpY,*pCmpU,*pCmpV)-*(pRef++)-corrs[1]);
                        diffB = abs(YUVJ2RGB_B(*(pCmpY++),*(pCmpU++),*(pCmpV++))-*(pRef++)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                    }
                    return( result );
                }
                default :
                    break;
            }
            break;
        }
        case FMT_RGB48 :
        {
            switch( image.mFormat )
            {
                case FMT_RGB :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(*pCmp++-*pRef-corrs[0]);
                        diffG = abs(*pCmp++-*(pRef+2)-corrs[1]);
                        diffB = abs(*pCmp++-*(pRef+4)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    unsigned short refR, refG, refB;
                    unsigned short cmpR, cmpG, cmpB;
                    unsigned short diffR, diffG, diffB;
                    short corrR16 = corrs[0]<<8;
                    short corrG16 = corrs[1]<<8;
                    short corrB16 = corrs[2]<<8;
                    while( pRef < mBuffer.tail() )
                    {
                        refR = (((unsigned short)*pRef)<<8) + *(pRef+1);
                        cmpR = (((unsigned short)*pCmp)<<8) + *(pCmp+1);
                        diffR = abs(refR-cmpR-corrR16);
                        *pDiff = (unsigned char)(diffR >> 8);
                        *(pDiff+1) = (unsigned char)diffR;
                        refG = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                        cmpG = (((unsigned short)*(pCmp+2))<<8) + *(pCmp+3);
                        diffG = abs(refG-cmpG-corrG16);
                        *pDiff = (unsigned char)(diffG >> 8);
                        *(pDiff+1) = (unsigned char)diffG;
                        refB = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                        cmpB = (((unsigned short)*(pCmp+4))<<8) + *(pCmp+5);
                        diffB = abs(refB-cmpB-corrB16);
                        *pDiff = (unsigned char)(diffB >> 8);
                        *(pDiff+1) = (unsigned char)diffB;
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUV2RGB_R(*pCmp,*(pCmp+1),*(pCmp+2))-*pRef-corrs[0]);
                        diffG = abs(YUV2RGB_G(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef+2)-corrs[1]);
                        diffB = abs(YUV2RGB_B(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef+4)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUVJ2RGB_R(*pCmp,*(pCmp+1),*(pCmp+2))-*pRef-corrs[0]);
                        diffG = abs(YUVJ2RGB_G(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef+2)-corrs[1]);
                        diffB = abs(YUVJ2RGB_B(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef+4)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    unsigned char *pCmpY = pCmp;
                    unsigned char *pCmpU = pCmpY+(image.mStride*image.mHeight);
                    unsigned char *pCmpV = pCmpU+(image.mStride*image.mHeight);
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUV2RGB_R(*pCmpY,*pCmpU,*pCmpV)-*pRef-corrs[0]);
                        diffG = abs(YUV2RGB_G(*pCmpY,*pCmpU,*pCmpV)-*(pRef+2)-corrs[1]);
                        diffB = abs(YUV2RGB_B(*(pCmpY++),*(pCmpU++),*(pCmpV++))-*(pRef+4)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    unsigned char *pCmpY = pCmp;
                    unsigned char *pCmpU = pCmpY+(image.mStride*image.mHeight);
                    unsigned char *pCmpV = pCmpU+(image.mStride*image.mHeight);
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUVJ2RGB_R(*pCmpY,*pCmpU,*pCmpV)-*pRef-corrs[0]);
                        diffG = abs(YUVJ2RGB_G(*pCmpY,*pCmpU,*pCmpV)-*(pRef+2)-corrs[1]);
                        diffB = abs(YUVJ2RGB_B(*(pCmpY++),*(pCmpU++),*(pCmpV++))-*(pRef+4)-corrs[2]);
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                    }
                    return( result );
                }
                default :
                    break;
            }
            break;
        }
        default :
            break;
    }
    Panic( "Unsupported delta of format %d with format %d", mFormat, image.mFormat );
    return( 0 );
}

/**
* @brief 
*
* @param image
* @param correct
*
* @return 
*/
Image *Image::delta2( const Image &image, bool correct ) const
{
    if ( !(mWidth == image.mWidth && mHeight == image.mHeight) )
        Panic( "Attempt to get delta of different sized images, expected %dx%d, got %dx%d", mWidth, mHeight, image.mWidth, image.mHeight );
    if ( mFormat == FMT_UNDEF || image.mFormat == FMT_UNDEF )
        Panic( "Attempt to get delta of image with undefined format" );

    if ( mColourSpace != CS_RGB || image.mChannels == 1 )
        return( yDelta( image, correct ) );

    Image *result = 0;
    if ( mFormat == FMT_RGB48 )
        result = new Image( FMT_GREY16, mWidth, mHeight );
    else
        result = new Image( FMT_GREY, mWidth, mHeight );

    const Image *cmpImage = &image;

    Debug( 5, "Diffing format %d with format %d to format %d", mFormat, image.mFormat, result->mFormat );
    unsigned char *pRef = mBuffer.data();
    unsigned char *pCmp = image.mBuffer.data();
    unsigned char *pDiff = result->mBuffer.data();

    if ( image.mColourSpace != CS_RGB )
    {
        Warning( "Correction disabled when comparing format %d with format %d", mFormat, image.mFormat );
        correct = false;
    }

    std::auto_ptr<Image> autoPtr( 0 );
    if ( correct )
    {
        if ( image.mColourSpace == CS_RGB )
        {
            unsigned long long refTotal = total();
            unsigned long long cmpTotal = image.total();
            double corr = (long double)cmpTotal/(long double)refTotal;
            int corrInt = (int)(corr*100.5);
            Debug( 5, "Applying correction of %.2f (%lld/%lld)", corr, cmpTotal, refTotal );
            if ( corrInt != 100 )
            {
                Debug( 5, "Correcting %d", corrInt );
                Image *adjImage = new Image( image );
                unsigned char *pAdj = adjImage->mBuffer.data();
                switch ( adjImage->mFormat )
                {
                    case FMT_RGB :
                    {
                        int r,g,b,y,u,v,r2,g2,b2;
                        while( pAdj < adjImage->mBuffer.tail() )
                        {
                            r = pAdj[0];
                            g = pAdj[1];
                            b = pAdj[2];
                            y = (Image::RGB2YUVJ_Y(r,g,b) * 100) / corrInt;
                            u = Image::RGB2YUVJ_U(r,g,b);
                            v = Image::RGB2YUVJ_V(r,g,b);
                            r2 = Image::YUVJ2RGB_R(y,u,v);
                            g2 = Image::YUVJ2RGB_G(y,u,v);
                            b2 = Image::YUVJ2RGB_B(y,u,v);
                            pAdj[0] = r2;
                            pAdj[1] = g2;
                            pAdj[2] = b2;
                            pAdj += adjImage->mPixelStep;
                        }
                        break;
                    }
                    default :
                    {
                        Panic( "Unsupported format %d for colour correction", mFormat );
                        break;
                    }
                }
                cmpImage = adjImage;
                autoPtr.reset( adjImage );
            }
        }
        else
        {
            unsigned long long refTotal = total();
            unsigned long long cmpTotal = image.total();
            double corr = (long double)cmpTotal/(long double)refTotal;
            int corrInt = (int)(corr*100.5);
            Debug( 5, "Applying correction of %.2f (%lld/%lld)", corr, cmpTotal, refTotal );
            if ( corrInt != 100 )
            {
                Debug( 5, "Correcting %d", corrInt );
                Image *adjImage = new Image( Image::FMT_RGB, image );
                unsigned char *pAdj = adjImage->mBuffer.data();
                while( pAdj < adjImage->mBuffer.tail() )
                    *pAdj++ = clamp( ((unsigned short)*pAdj * 100) / corrInt, 0, 255 );
                cmpImage = adjImage;
                autoPtr.reset( adjImage );
            }
        }
    }

    switch( mFormat )
    {
        case FMT_RGB :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_RGB :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(*pCmp++-*pRef++);
                        diffG = abs(*pCmp++-*pRef++);
                        diffB = abs(*pCmp++-*pRef++);
                        *pDiff++ = diffR+diffG+diffB/3;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(*pCmp-*pRef++);
                        diffG = abs(*(pCmp+2)-*pRef++);
                        diffB = abs(*(pCmp+4)-*pRef++);
                        *pDiff++ = diffR+diffG+diffB/3;
                        pCmp += cmpImage->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUV2RGB_R(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++));
                        diffG = abs(YUV2RGB_G(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++));
                        diffB = abs(YUV2RGB_B(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++));
                        *pDiff++ = diffR+diffG+diffB/3;
                        pCmp += cmpImage->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUVJ2RGB_R(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++));
                        diffG = abs(YUVJ2RGB_G(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++));
                        diffB = abs(YUVJ2RGB_B(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef++));
                        *pDiff++ = diffR+diffG+diffB/3;
                        pCmp += cmpImage->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    unsigned char *pCmpY = pCmp;
                    unsigned char *pCmpU = pCmpY+(cmpImage->mStride*cmpImage->mHeight);
                    unsigned char *pCmpV = pCmpU+(cmpImage->mStride*cmpImage->mHeight);
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUV2RGB_R(*pCmpY,*pCmpU,*pCmpV)-*(pRef++));
                        diffG = abs(YUV2RGB_G(*pCmpY,*pCmpU,*pCmpV)-*(pRef++));
                        diffB = abs(YUV2RGB_B(*(pCmpY++),*(pCmpU++),*(pCmpV++))-*(pRef++));
                        *pDiff++ = diffR+diffG+diffB/3;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    unsigned char *pCmpY = pCmp;
                    unsigned char *pCmpU = pCmpY+(cmpImage->mStride*cmpImage->mHeight);
                    unsigned char *pCmpV = pCmpU+(cmpImage->mStride*cmpImage->mHeight);
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUVJ2RGB_R(*pCmpY,*pCmpU,*pCmpV)-*(pRef++));
                        diffG = abs(YUVJ2RGB_G(*pCmpY,*pCmpU,*pCmpV)-*(pRef++));
                        diffB = abs(YUVJ2RGB_B(*(pCmpY++),*(pCmpU++),*(pCmpV++))-*(pRef++));
                        *pDiff++ = diffR+diffG+diffB/3;
                    }
                    return( result );
                }
                default :
                    break;
            }
            break;
        }
        case FMT_RGB48 :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_RGB :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(*pCmp++-*pRef);
                        diffG = abs(*pCmp++-*(pRef+2));
                        diffB = abs(*pCmp++-*(pRef+4));
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    unsigned short refR, refG, refB;
                    unsigned short cmpR, cmpG, cmpB;
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        refR = (((unsigned short)*pRef)<<8) + *(pRef+1);
                        cmpR = (((unsigned short)*pCmp)<<8) + *(pCmp+1);
                        diffR = abs(refR-cmpR);
                        *pDiff = (unsigned char)(diffR >> 8);
                        *(pDiff+1) = (unsigned char)diffR;
                        refG = (((unsigned short)*(pRef+2))<<8) + *(pRef+3);
                        cmpG = (((unsigned short)*(pCmp+2))<<8) + *(pCmp+3);
                        diffG = abs(refG-cmpG);
                        *pDiff = (unsigned char)(diffG >> 8);
                        *(pDiff+1) = (unsigned char)diffG;
                        refB = (((unsigned short)*(pRef+4))<<8) + *(pRef+5);
                        cmpB = (((unsigned short)*(pCmp+4))<<8) + *(pCmp+5);
                        diffB = abs(refB-cmpB);
                        *pDiff = (unsigned char)(diffB >> 8);
                        *(pDiff+1) = (unsigned char)diffB;
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUV2RGB_R(*pCmp,*(pCmp+1),*(pCmp+2))-*pRef);
                        diffG = abs(YUV2RGB_G(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef+2));
                        diffB = abs(YUV2RGB_B(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef+4));
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUVJ2RGB_R(*pCmp,*(pCmp+1),*(pCmp+2))-*pRef);
                        diffG = abs(YUVJ2RGB_G(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef+2));
                        diffB = abs(YUVJ2RGB_B(*pCmp,*(pCmp+1),*(pCmp+2))-*(pRef+4));
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    unsigned char *pCmpY = pCmp;
                    unsigned char *pCmpU = pCmpY+(cmpImage->mStride*cmpImage->mHeight);
                    unsigned char *pCmpV = pCmpU+(cmpImage->mStride*cmpImage->mHeight);
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUV2RGB_R(*pCmpY,*pCmpU,*pCmpV)-*pRef);
                        diffG = abs(YUV2RGB_G(*pCmpY,*pCmpU,*pCmpV)-*(pRef+2));
                        diffB = abs(YUV2RGB_B(*(pCmpY++),*(pCmpU++),*(pCmpV++))-*(pRef+4));
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    unsigned char *pCmpY = pCmp;
                    unsigned char *pCmpU = pCmpY+(cmpImage->mStride*cmpImage->mHeight);
                    unsigned char *pCmpV = pCmpU+(cmpImage->mStride*cmpImage->mHeight);
                    unsigned short diffR, diffG, diffB;
                    while( pRef < mBuffer.tail() )
                    {
                        diffR = abs(YUVJ2RGB_R(*pCmpY,*pCmpU,*pCmpV)-*pRef);
                        diffG = abs(YUVJ2RGB_G(*pCmpY,*pCmpU,*pCmpV)-*(pRef+2));
                        diffB = abs(YUVJ2RGB_B(*(pCmpY++),*(pCmpU++),*(pCmpV++))-*(pRef+4));
                        *pDiff++ = diffR+diffG+diffB/3;
                        *pDiff++ = 0;
                        pRef += mPixelStep;
                    }
                    return( result );
                }
                default :
                    break;
            }
            break;
        }
        default :
            break;
    }
    Panic( "Unsupported delta of format %d with format %d", mFormat, image.mFormat );
    return( 0 );
}

/**
* @brief 
*
* @param image
* @param correct
*
* @return 
*/
Image *Image::yDelta( const Image &image, bool correct ) const
{
    if ( !(mWidth == image.mWidth && mHeight == image.mHeight) )
        Panic( "Attempt to get delta of different sized images, expected %dx%d, got %dx%d", mWidth, mHeight, image.mWidth, image.mHeight );
    if ( mFormat == FMT_UNDEF || image.mFormat == FMT_UNDEF )
        Panic( "Attempt to get delta of image with undefined format" );

    Image *result = 0;
    if ( mFormat == FMT_GREY16 )
        result = new Image( FMT_GREY16, mWidth, mHeight );
    else
        result = new Image( FMT_GREY, mWidth, mHeight );

    Debug( 5, "yDiffing format %d with format %d to format %d", mFormat, image.mFormat, result->mFormat );
    unsigned char *pRef = mBuffer.data();
    unsigned char *pCmp = image.mBuffer.data();
    unsigned char *pDiff = result->mBuffer.data();

    int corr = 0;

    if ( correct )
    {
        unsigned long long refTotal = total();
        unsigned long long cmpTotal = image.total();
        corr = ::llabs((long long int)(cmpTotal - refTotal)) / mPixels;
        Debug( 5, "Applying correction of %d (%lld-%lld=%lld/%d)", corr, refTotal, cmpTotal, refTotal-cmpTotal, mPixels );
    }

    switch( mFormat )
    {
        case FMT_GREY :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_GREY16 :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_GREY16 :
                {
                    short corr16 = corr<<8;
                    while( pRef < mBuffer.tail() )
                    {
                        unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                        unsigned short cmp = (((unsigned short)*pCmp)<<8) + *(pCmp+1);
                        unsigned short diff = abs(ref-cmp-corr16);
                        *pDiff = (unsigned char)(diff >> 8);
                        *(pDiff+1) = (unsigned char)diff;
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp += image.mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pRef += mPixelStep;
                        pCmp++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_RGB :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2))-corr);
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_RGB48 :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4))-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+2))-corr);
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUV :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVJ :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef-corr);
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVP :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_YUVJ :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_GREY16 :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    unsigned char *pCmpU = pCmp+(image.mStride*image.mHeight);
                    while( pCmp < pCmpU )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp++;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    unsigned char *pCmpU = pCmp+(image.mStride*image.mHeight);
                    while( pCmp < pCmpU )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef)-corr);
                        pCmp++;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVJP :
        {
            switch( image.mFormat )
            {
                case FMT_GREY :
                case FMT_YUVJ :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_GREY16 :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pCmp < image.mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef-corr);
                        pCmp += image.mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    unsigned char *pCmpU = pCmp+(image.mStride*image.mHeight);
                    while( pCmp < pCmpU )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef-corr);
                        pCmp++;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    unsigned char *pCmpU = pCmp+(image.mStride*image.mHeight);
                    while( pCmp < pCmpU )
                    {
                        *pDiff = abs(*pCmp-*pRef-corr);
                        pCmp++;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_UNDEF :
            break;
    }
    Panic( "Unsupported yDelta of format %d with format %d", mFormat, image.mFormat );
    return( 0 );
}

/**
* @brief 
*
* @param image
* @param correct
*
* @return 
*/
Image *Image::yDelta2( const Image &image, bool correct ) const
{
    if ( !(mWidth == image.mWidth && mHeight == image.mHeight) )
        Panic( "Attempt to get delta of different sized images, expected %dx%d, got %dx%d", mWidth, mHeight, image.mWidth, image.mHeight );
    if ( mFormat == FMT_UNDEF || image.mFormat == FMT_UNDEF )
        Panic( "Attempt to get delta of image with undefined format" );

    const Image *cmpImage = &image;

    Image *result = 0;
    if ( mFormat == FMT_GREY16 )
        result = new Image( FMT_GREY16, mWidth, mHeight );
    else
        result = new Image( FMT_GREY, mWidth, mHeight );

    Debug( 5, "yDiffing format %d with format %d to format %d", mFormat, image.mFormat, result->mFormat );

    std::auto_ptr<Image> autoPtr( 0 );
    if ( correct )
    {
        unsigned long long refTotal = total();
        unsigned long long cmpTotal = image.total();
        double corr = (long double)cmpTotal/(long double)refTotal;
        int corrInt = (int)(corr*100.5);
        Debug( 5, "Applying correction of %.2f (%lld/%lld)", corr, cmpTotal, refTotal );
        if ( corrInt != 100 )
        {
            Debug( 5, "Correcting %d", corrInt );
            Image *adjImage = new Image( Image::FMT_GREY, image );
            unsigned char *pAdj = adjImage->mBuffer.data();
            while( pAdj < adjImage->mBuffer.tail() )
                *pAdj++ = clamp( ((unsigned short)*pAdj * 100) / corrInt, 0, 255 );
            cmpImage = adjImage;
            autoPtr.reset( adjImage );
        }
    }

    unsigned char *pRef = mBuffer.data();
    unsigned char *pCmp = cmpImage->mBuffer.data();
    unsigned char *pDiff = result->mBuffer.data();

    switch( mFormat )
    {
        case FMT_GREY :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef);
                        pRef += mPixelStep;
                        pCmp++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pRef += mPixelStep;
                        pCmp++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_GREY16 :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_GREY :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        unsigned short ref = (((unsigned short)*pRef)<<8) + *(pRef+1);
                        unsigned short cmp = (((unsigned short)*pCmp)<<8) + *(pCmp+1);
                        unsigned short diff = abs(ref-cmp);
                        *pDiff = (unsigned char)(diff >> 8);
                        *(pDiff+1) = (unsigned char)diff;
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pRef += mPixelStep;
                        pCmp += cmpImage->mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef);
                        pRef += mPixelStep;
                        pCmp++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pRef += mPixelStep;
                        pCmp++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_RGB :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-RGB2YUVJ_Y(*(pRef),*(pRef+1),*(pRef+2)));
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_RGB48 :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+4)));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-RGB2YUVJ_Y(*(pRef),*(pRef+2),*(pRef+2)));
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUV :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-YUV2YUVJ_Y(*pRef));
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef));
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVJ :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_GREY :
                case FMT_GREY16 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJ :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef);
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    while( pRef < mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pCmp++;
                        pRef += mPixelStep;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVP :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_GREY :
                case FMT_YUVJ :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_GREY16 :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-YUV2YUVJ_Y(*pRef));
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    unsigned char *pCmpU = pCmp+(cmpImage->mStride*cmpImage->mHeight);
                    while( pCmp < pCmpU )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-YUV2YUVJ_Y(*pRef));
                        pCmp++;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    unsigned char *pCmpU = pCmp+(cmpImage->mStride*cmpImage->mHeight);
                    while( pCmp < pCmpU )
                    {
                        *pDiff = abs(*pCmp-YUV2YUVJ_Y(*pRef));
                        pCmp++;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_YUVJP :
        {
            switch( cmpImage->mFormat )
            {
                case FMT_GREY :
                case FMT_YUVJ :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_GREY16 :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+1),*(pCmp+2))-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_RGB48 :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(RGB2YUVJ_Y(*(pCmp),*(pCmp+2),*(pCmp+4))-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUV :
                {
                    while( pCmp < cmpImage->mBuffer.tail() )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef);
                        pCmp += cmpImage->mPixelStep;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVP :
                {
                    unsigned char *pCmpU = pCmp+(cmpImage->mStride*cmpImage->mHeight);
                    while( pCmp < pCmpU )
                    {
                        *pDiff = abs(YUV2YUVJ_Y(*pCmp)-*pRef);
                        pCmp++;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_YUVJP :
                {
                    unsigned char *pCmpU = pCmp+(cmpImage->mStride*cmpImage->mHeight);
                    while( pCmp < pCmpU )
                    {
                        *pDiff = abs(*pCmp-*pRef);
                        pCmp++;
                        pRef++;
                        pDiff += result->mPixelStep;
                    }
                    return( result );
                }
                case FMT_UNDEF :
                    break;
            }
            break;
        }
        case FMT_UNDEF :
            break;
    }
    Panic( "Unsupported yDelta of format %d with format %d", mFormat, image.mFormat );
    return( 0 );
}

/**
* @brief 
*
* @param text
*
* @return 
*/
const Coord Image::textCentreCoord( const char *text )
{
    int index = 0;
    int lineNo = 0;
    int textLen = strlen( text );
    int lineLen = 0;
    int maxLineLen = 0;
    const char *line = text;

    while ( (index < textLen) && (lineLen = strcspn( line, "\n" )) )
    {
        if ( lineLen > maxLineLen )
            maxLineLen = lineLen;

        index += lineLen;
        while ( text[index] == '\n' )
        {
            index++;
        }
        line = text+index;
        lineNo++;
    }
    int x = (mWidth - (maxLineLen * OZ_CHAR_WIDTH) ) / 2;
    int y = (mHeight - (lineNo * LINE_HEIGHT) ) / 2;
    return( Coord( x, y ) );
}

/**
* @brief 
*
* @param text
* @param coord
* @param fgColour
* @param bgColour
*/
void Image::annotate( const char *text, const Coord &coord, const Rgb fgColour, const Rgb bgColour )
{
    mText = text;

    int index = 0;
    int lineNo = 0;
    int textLen = mText.length();
    int lineLen = 0;
    const char *line = mText.c_str();

    unsigned char fgColR = RGB_RED_VAL(fgColour);
    unsigned char fgColG = RGB_GREEN_VAL(fgColour);
    unsigned char fgColB = RGB_BLUE_VAL(fgColour);
    bool fgTrans = (fgColour == RGB_TRANSPARENT);

    unsigned char bgColR = RGB_RED_VAL(bgColour);
    unsigned char bgColG = RGB_GREEN_VAL(bgColour);
    unsigned char bgColB = RGB_BLUE_VAL(bgColour);
    bool bgTrans = (bgColour == RGB_TRANSPARENT);

    while ( (index < textLen) && (lineLen = strcspn( line, "\n" )) )
    {
        int lineWidth = lineLen * OZ_CHAR_WIDTH;

        int loLineX = coord.x();
        int loLineY = coord.y() + (lineNo * LINE_HEIGHT);

        int minLineX = 0;
        int maxLineX = mWidth - lineWidth;
        int minLineY = 0;
        int maxLineY = mHeight - LINE_HEIGHT;

        if ( loLineX > maxLineX )
            loLineX = maxLineX;
        if ( loLineX < minLineX )
            loLineX = minLineX;
        if ( loLineY > maxLineY )
            loLineY = maxLineY;
        if ( loLineY < minLineY )
            loLineY = minLineY;

        int hiLineX = loLineX + lineWidth;
        int hiLineY = loLineY + LINE_HEIGHT;

        // Clip anything that runs off the right of the screen
        if ( hiLineX > mWidth )
            hiLineX = mWidth;
        if ( hiLineY > mHeight )
            hiLineY = mHeight;

        switch( mFormat )
        {
            case FMT_GREY :
            case FMT_GREY16 :
            {
                unsigned char fgColY = RGB2YUVJ_Y(fgColR,fgColG,fgColB);
                unsigned char bgColY = RGB2YUVJ_Y(bgColR,bgColG,bgColB);
                unsigned char *ptr = mBuffer.data()+((loLineY*mStride)+(loLineX*mPixelStep));
                for ( int y = loLineY, row = 0; y < hiLineY && row < OZ_CHAR_HEIGHT; y++, row++, ptr += mStride )
                {
                    unsigned char *tempPtr = ptr;
                    for ( int x = loLineX, c = 0; x < hiLineX && c < lineLen; c++ )
                    {
                        int f = fontdata[(line[c] * OZ_CHAR_HEIGHT) + row];
                        for ( int i = 0; i < OZ_CHAR_WIDTH && x < hiLineX; i++, x++, tempPtr += mPixelStep )
                        {
                            if ( f & (0x80 >> i) )
                            {
                                if ( !fgTrans )
                                    *tempPtr = fgColY;
                            }
                            else if ( !bgTrans )
                            {
                                *tempPtr = bgColY;
                            }
                        }
                    }
                }
                break;
            }
            case FMT_RGB :
            {
                unsigned char *ptr = mBuffer.data()+(((loLineY*mWidth)+loLineX)*mChannels);
                for ( int y = loLineY, row = 0; y < hiLineY && row < OZ_CHAR_HEIGHT; y++, row++, ptr += mStride )
                {
                    unsigned char *tempPtr = ptr;
                    for ( int x = loLineX, c = 0; x < hiLineX && c < lineLen; c++ )
                    {
                        int f = fontdata[(line[c] * OZ_CHAR_HEIGHT) + row];
                        for ( int i = 0; i < OZ_CHAR_WIDTH && x < hiLineX; i++, x++, tempPtr += mPixelStep )
                        {
                            if ( f & (0x80 >> i) )
                            {
                                if ( !fgTrans )
                                {
                                    RED(tempPtr) = fgColR;
                                    GREEN(tempPtr) = fgColG;
                                    BLUE(tempPtr) = fgColB;
                                }
                            }
                            else if ( !bgTrans )
                            {
                                RED(tempPtr) = bgColR;
                                GREEN(tempPtr) = bgColG;
                                BLUE(tempPtr) = bgColB;
                            }
                        }
                    }
                }
                break;
            }
            case FMT_RGB48 :
            {
                unsigned char *ptr = mBuffer.data()+(((loLineY*mWidth)+loLineX)*mChannels);
                for ( int y = loLineY, row = 0; y < hiLineY && row < OZ_CHAR_HEIGHT; y++, row++, ptr += mStride )
                {
                    unsigned char *tempPtr = ptr;
                    for ( int x = loLineX, c = 0; x < hiLineX && c < lineLen; c++ )
                    {
                        int f = fontdata[(line[c] * OZ_CHAR_HEIGHT) + row];
                        for ( int i = 0; i < OZ_CHAR_WIDTH && x < hiLineX; i++, x++, tempPtr += mPixelStep )
                        {
                            if ( f & (0x80 >> i) )
                            {
                                if ( !fgTrans )
                                {
                                    RED16(tempPtr) = fgColR;
                                    GREEN16(tempPtr) = fgColG;
                                    BLUE16(tempPtr) = fgColB;
                                    *(tempPtr+1) = *(tempPtr+3) = *(tempPtr+5) = 0;
                                }
                            }
                            else if ( !bgTrans )
                            {
                                RED16(tempPtr) = bgColR;
                                GREEN16(tempPtr) = bgColG;
                                BLUE16(tempPtr) = bgColB;
                                *(tempPtr+1) = *(tempPtr+3) = *(tempPtr+5) = 0;
                            }
                        }
                    }
                }
                break;
            }
            case FMT_YUV :
            case FMT_YUVJ :
            {
                RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
                unsigned char fgColY = convFuncs.Y(fgColR,fgColG,fgColB);
                unsigned char fgColU = convFuncs.U(fgColR,fgColG,fgColB);
                unsigned char fgColV = convFuncs.V(fgColR,fgColG,fgColB);
                unsigned char bgColY = convFuncs.Y(bgColR,bgColG,bgColB);
                unsigned char bgColU = convFuncs.U(bgColR,bgColG,bgColB);
                unsigned char bgColV = convFuncs.V(bgColR,bgColG,bgColB);
                unsigned char *ptr = mBuffer.data()+(((loLineY*mWidth)+loLineX)*mChannels);
                for ( int y = loLineY, row = 0; y < hiLineY && row < OZ_CHAR_HEIGHT; y++, row++, ptr += mStride )
                {
                    unsigned char *tempPtr = ptr;
                    for ( int x = loLineX, c = 0; x < hiLineX && c < lineLen; c++ )
                    {
                        int f = fontdata[(line[c] * OZ_CHAR_HEIGHT) + row];
                        for ( int i = 0; i < OZ_CHAR_WIDTH && x < hiLineX; i++, x++, tempPtr += mPixelStep )
                        {
                            if ( f & (0x80 >> i) )
                            {
                                if ( !fgTrans )
                                {
                                    *(tempPtr) = fgColY;
                                    *(tempPtr+1) = fgColU;
                                    *(tempPtr+2) = fgColV;
                                }
                            }
                            else if ( !bgTrans )
                            {
                                *(tempPtr) = bgColY;
                                *(tempPtr+1) = bgColU;
                                *(tempPtr+2) = bgColV;
                            }
                        }
                    }
                }
                break;
            }
            case FMT_YUVP :
            case FMT_YUVJP :
            {
                RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
                unsigned char fgColY = convFuncs.Y(fgColR,fgColG,fgColB);
                unsigned char fgColU = convFuncs.U(fgColR,fgColG,fgColB);
                unsigned char fgColV = convFuncs.V(fgColR,fgColG,fgColB);
                unsigned char bgColY = convFuncs.Y(bgColR,bgColG,bgColB);
                unsigned char bgColU = convFuncs.U(bgColR,bgColG,bgColB);
                unsigned char bgColV = convFuncs.V(bgColR,bgColG,bgColB);
                unsigned char *pDstY = mBuffer.data()+((loLineY*mWidth)+loLineX);
                for ( int y = loLineY, row = 0; y < hiLineY && row < OZ_CHAR_HEIGHT; y++, row++, pDstY += mStride )
                {
                    unsigned char *tempDstY = pDstY;
                    unsigned char *tempDstU = tempDstY + mPlaneSize;
                    unsigned char *tempDstV = tempDstU + mPlaneSize;
                    for ( int x = loLineX, c = 0; x < hiLineX && c < lineLen; c++ )
                    {
                        int f = fontdata[(line[c] * OZ_CHAR_HEIGHT) + row];
                        for ( int i = 0; i < OZ_CHAR_WIDTH && x < hiLineX; i++, x++, tempDstY += mPixelStep, tempDstU += mPixelStep, tempDstV += mPixelStep )
                        {
                            if ( f & (0x80 >> i) )
                            {
                                if ( !fgTrans )
                                {
                                    *tempDstY = fgColY;
                                    *tempDstU = fgColU;
                                    *tempDstV = fgColV;
                                }
                            }
                            else if ( !bgTrans )
                            {
                                *tempDstY = bgColY;
                                *tempDstU = bgColU;
                                *tempDstV = bgColV;
                            }
                        }
                    }
                }
                break;
            }
            default :
            {
                throw Exception( stringtf( "Unexpected format %d found when annotating image", mFormat ) );
                break;
            }
        }
        index += lineLen;
        while ( text[index] == '\n' )
        {
            index++;
        }
        line = text+index;
        lineNo++;
    }
}

/**
* @brief 
*
* @param label
* @param when
* @param coord
*/
void Image::timestamp( const char *label, const time_t when, const Coord &coord )
{
    char timeText[256];
    strftime( timeText, sizeof(timeText), "%y/%m/%d %H:%M:%S", localtime( &when ) );
    char text[64];
    if ( label )
    {
        snprintf( text, sizeof(text), "%s - %s", label, timeText );
        annotate( text, coord );
    }
    else
    {
        annotate( timeText, coord );
    }
}

/**
* @brief 
*/
void Image::erase()
{
    switch( mFormat )
    {
        case FMT_GREY :
        case FMT_GREY16 :
        case FMT_RGB :
        case FMT_RGB48 :
        case FMT_YUVJ :
        case FMT_YUVJP :
        {
            mBuffer.erase();
            return;
        }
        case FMT_YUV :
        case FMT_YUVP :
        {
            mBuffer.fill( 16 );
            return;
        }
        case FMT_UNDEF :
        default :
        {
            break;
        }
    }
    Panic( "Unsupported erase of format %d", mFormat );
}

/**
* @brief 
*
* @param colour
* @param density
*/
void Image::fill( Rgb colour, unsigned char density )
{
    fill( colour, Box( 0, 0, mWidth-1, mHeight-1 ), density );
}

/**
* @brief 
*
* @param colour
* @param point
*/
void Image::fill( Rgb colour, const Coord &point )
{
    int x = point.x();
    int y = point.y();

    unsigned char colR = RGB_RED_VAL(colour);
    unsigned char colG = RGB_GREEN_VAL(colour);
    unsigned char colB = RGB_BLUE_VAL(colour);

    switch( mFormat )
    {
        case FMT_GREY :
        {
            unsigned char colY;
            switch( colour )
            {
                case RGB_WHITE :
                    colY = WHITE;
                    break;
                case RGB_BLACK :
                    colY = BLACK;
                    break;
                default :
                    colY = RGB2YUVJ_Y(colR,colG,colB);
                    break;
            }
            unsigned char *p = mBuffer.data()+((y*mWidth)+x);
            *p = colY;
            break;
        }
        case FMT_GREY16 :
        {
            unsigned char colY;
            switch( colour )
            {
                case RGB_WHITE :
                    colY = WHITE;
                    break;
                case RGB_BLACK :
                    colY = BLACK;
                    break;
                default :
                    colY = RGB2YUVJ_Y(colR,colG,colB);
                    break;
            }
            unsigned char *p = mBuffer.data()+(mPixelStep*((y*mWidth)+x));
            *p = colY;
            *(p+1) = 0;
            break;
        }
        case FMT_RGB :
        {
            unsigned char *p = mBuffer.data()+(mPixelStep*((y*mWidth)+x));
            RED(p) = colR;
            GREEN(p) = colG;
            BLUE(p) = colB;
            break;
        }
        case FMT_RGB48 :
        {
            unsigned char *p = mBuffer.data()+(mPixelStep*((y*mWidth)+x));
            RED16(p) = colR;
            GREEN16(p) = colG;
            BLUE16(p) = colB;
            *(p+1) = *(p+3) = *(p+5) = 0;
            break;
        }
        case FMT_YUV :
        case FMT_YUVJ :
        {
            RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
            unsigned char colY = convFuncs.Y(colR,colG,colB);
            unsigned char colU = convFuncs.U(colR,colG,colB);
            unsigned char colV = convFuncs.V(colR,colG,colB);
            unsigned char *p = mBuffer.data()+(mPixelStep*((y*mWidth)+x));
            *(p) = colY;
            *(p+1) = colU;
            *(p+2) = colV;
            break;
        }
        case FMT_YUVP :
        case FMT_YUVJP :
        {
            RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
            unsigned char colY = convFuncs.Y(colR,colG,colB);
            unsigned char colU = convFuncs.U(colR,colG,colB);
            unsigned char colV = convFuncs.V(colR,colG,colB);
            unsigned char *pY = mBuffer.data()+(mPixelStep*((y*mWidth)+x));
            unsigned char *pU = pY + mPlaneSize;
            unsigned char *pV = pU + mPlaneSize;
            *pY = colY;
            *pU = colU;
            *pV = colV;
            break;
        }
        default :
        {
            throw Exception( stringtf( "Unexpected format %d found when filling image", mFormat ) );
            break;
        }
    }
}

/**
* @brief 
*
* @param colour
* @param limits
* @param density
*/
void Image::fill( Rgb colour, const Box &limits, unsigned char density )
{
    int loX = limits.lo().x();
    int loY = limits.lo().y();
    int hiX = limits.hi().x();
    int hiY = limits.hi().y();

    unsigned char colR = RGB_RED_VAL(colour);
    unsigned char colG = RGB_GREEN_VAL(colour);
    unsigned char colB = RGB_BLUE_VAL(colour);

    switch( mFormat )
    {
        case FMT_GREY :
        {
            unsigned char colY;
            switch( colour )
            {
                case RGB_WHITE :
                    colY = WHITE;
                    break;
                case RGB_BLACK :
                    colY = BLACK;
                    break;
                default :
                    colY = RGB2YUVJ_Y(colR,colG,colB);
                    break;
            }
            if ( density == 1 )
            {
                mBuffer.fill( colY );
                break;
            }
            for ( int y = loY; y <= hiY; y++ )
            {
                unsigned char *p = mBuffer.data()+((y*mWidth)+loX);
                for ( int x = loX; x <= hiX; x++ )
                {
                    if ( (density <= 1) || (( x == loX || x == hiX || y == loY || y == hiY ) || (!(x%density) && !(y%density) )) )
                        *p = colY;
                    p++;
                }
            }
            break;
        }
        case FMT_GREY16 :
        {
            unsigned char colY;
            switch( colour )
            {
                case RGB_WHITE :
                    colY = WHITE;
                    break;
                case RGB_BLACK :
                    colY = BLACK;
                    break;
                default :
                    colY = RGB2YUVJ_Y(colR,colG,colB);
                    break;
            }
            for ( int y = loY; y <= hiY; y++ )
            {
                unsigned char *p = mBuffer.data()+(mPixelStep*((y*mWidth)+loX));
                for ( int x = loX; x <= hiX; x++ )
                {
                    if ( (density <= 1) || (( x == loX || x == hiX || y == loY || y == hiY ) || (!(x%density) && !(y%density) )) )
                    {
                        *p = colY;
                        *(p+1) = 0;
                    }
                    p += mPixelStep;
                }
            }
            break;
        }
        case FMT_RGB :
        {
            for ( int y = loY; y <= hiY; y++ )
            {
                unsigned char *p = mBuffer.data()+(mPixelStep*((y*mWidth)+loX));
                for ( int x = loX; x <= hiX; x++ )
                {
                    if ( (density <= 1) || (( x == loX || x == hiX || y == loY || y == hiY ) || (!(x%density) && !(y%density) )) )
                    {
                        RED(p) = colR;
                        GREEN(p) = colG;
                        BLUE(p) = colB;
                    }
                    p += mPixelStep;
                }
            }
            break;
        }
        case FMT_RGB48 :
        {
            for ( int y = loY; y <= hiY; y++ )
            {
                unsigned char *p = mBuffer.data()+(mPixelStep*((y*mWidth)+loX));
                for ( int x = loX; x <= hiX; x++ )
                {
                    if ( (density <= 1) || (( x == loX || x == hiX || y == loY || y == hiY ) || (!(x%density) && !(y%density) )) )
                    {
                        RED16(p) = colR;
                        GREEN16(p) = colG;
                        BLUE16(p) = colB;
                        *(p+1) = *(p+3) = *(p+5) = 0;
                    }
                    p += mPixelStep;
                }
            }
            break;
        }
        case FMT_YUV :
        case FMT_YUVJ :
        {
            RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
            unsigned char colY = convFuncs.Y(colR,colG,colB);
            unsigned char colU = convFuncs.U(colR,colG,colB);
            unsigned char colV = convFuncs.V(colR,colG,colB);
            for ( int y = loY; y <= hiY; y++ )
            {
                unsigned char *p = mBuffer.data()+(mPixelStep*((y*mWidth)+loX));
                for ( int x = loX; x <= hiX; x++ )
                {
                    if ( (density <= 1) || (( x == loX || x == hiX || y == loY || y == hiY ) || (!(x%density) && !(y%density) )) )
                    {
                        *(p) = colY;
                        *(p+1) = colU;
                        *(p+2) = colV;
                    }
                    p += mPixelStep;
                }
            }
            break;
        }
        case FMT_YUVP :
        case FMT_YUVJP :
        {
            RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
            unsigned char colY = convFuncs.Y(colR,colG,colB);
            unsigned char colU = convFuncs.U(colR,colG,colB);
            unsigned char colV = convFuncs.V(colR,colG,colB);
            for ( int y = loY; y <= hiY; y++ )
            {
                unsigned char *pY = mBuffer.data()+(mPixelStep*((y*mWidth)+loX));
                unsigned char *pU = pY + mPlaneSize;
                unsigned char *pV = pU + mPlaneSize;
                for ( int x = loX; x <= hiX; x++ )
                {
                    if ( (density <= 1) || (( x == loX || x == hiX || y == loY || y == hiY ) || (!(x%density) && !(y%density) )) )
                    {
                        *pY = colY;
                        *pU = colU;
                        *pV = colV;
                    }
                    pY += mPixelStep;
                    pU += mPixelStep;
                    pV += mPixelStep;
                }
            }
            break;
        }
        default :
        {
            throw Exception( stringtf( "Unexpected format %d found when filling image", mFormat ) );
            break;
        }
    }
}

/**
* @brief 
*
* @param colour
* @param polygon
* @param density
*/
void Image::fill( Rgb colour, const Polygon &polygon, unsigned char density )
{
    unsigned char colR = RGB_RED_VAL(colour);
    unsigned char colG = RGB_GREEN_VAL(colour);
    unsigned char colB = RGB_BLUE_VAL(colour);

    RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
    unsigned char colY = convFuncs.Y(colR,colG,colB);
    unsigned char colU = convFuncs.U(colR,colG,colB);
    unsigned char colV = convFuncs.V(colR,colG,colB);

    if ( mFormat == FMT_GREY || mFormat == FMT_GREY16 || mFormat == FMT_YUVJ || mFormat == FMT_YUVJP )
    {
        if ( colour == RGB_WHITE )
            colY = WHITE;
        else if ( colour == RGB_BLACK )
            colY = BLACK;
    }

    int numCoords = polygon.numCoords();
    int numGlobalEdges = 0;
    Edge globalEdges[numCoords];
    for ( int j = 0, i = numCoords-1; j < numCoords; i = j++ )
    {
        const Coord &p1 = polygon.coord( i );
        const Coord &p2 = polygon.coord( j );

        int x1 = p1.x();
        int x2 = p2.x();
        int y1 = p1.y();
        int y2 = p2.y();

        Debug( 9, "x1:%d,y1:%d x2:%d,y2:%d", x1, y1, x2, y2 );
        if ( y1 == y2 )
            continue;

        double dx = x2 - x1;
        double dy = y2 - y1;

        globalEdges[numGlobalEdges].minY = y1<y2?y1:y2;
        globalEdges[numGlobalEdges].maxY = y1<y2?y2:y1;
        globalEdges[numGlobalEdges].minX = y1<y2?x1:x2;
        globalEdges[numGlobalEdges]._1_m = dx/dy;
        numGlobalEdges++;
    }
    qsort( globalEdges, numGlobalEdges, sizeof(*globalEdges), Edge::CompareYX );

#ifndef DBG_OFF
    if ( dbgLevel >= 9 )
    {
        for ( int i = 0; i < numGlobalEdges; i++ )
        {
            Debug( 9, "%d: minY: %d, maxY:%d, minX:%.2f, 1/m:%.2f", i, globalEdges[i].minY, globalEdges[i].maxY, globalEdges[i].minX, globalEdges[i]._1_m );
        }
    }
#endif

    int numActiveEdges = 0;
    Edge activeEdges[numGlobalEdges];
    int y = globalEdges[0].minY;
    do 
    {
        for ( int i = 0; i < numGlobalEdges; i++ )
        {
            if ( globalEdges[i].minY == y )
            {
                Debug( 9, "Moving global edge" );
                activeEdges[numActiveEdges++] = globalEdges[i];
                if ( i < (numGlobalEdges-1) )
                {
                    memmove( &globalEdges[i], &globalEdges[i+1], sizeof(*globalEdges)*(numGlobalEdges-i) );
                    i--;
                }
                numGlobalEdges--;
            }
            else
            {
                break;
            }
        }
        qsort( activeEdges, numActiveEdges, sizeof(*activeEdges), Edge::CompareX );
#ifndef DBG_OFF
        if ( dbgLevel >= 9 )
        {
            for ( int i = 0; i < numActiveEdges; i++ )
            {
                Debug( 9, "%d - %d: minY: %d, maxY:%d, minX:%.2f, 1/m:%.2f", y, i, activeEdges[i].minY, activeEdges[i].maxY, activeEdges[i].minX, activeEdges[i]._1_m );
            }
        }
#endif
        if ( !(y%density) )
        {
            //Debug( 9, "%d", y );
            for ( int i = 0; i < numActiveEdges; )
            {
                int loX = int(round(activeEdges[i++].minX));
                int hiX = int(round(activeEdges[i++].minX));
                unsigned char *p = mBuffer.data()+((y*mStride)+(loX*mPixelStep));
                for ( int x = loX; x <= hiX; x++, p += mPixelStep )
                {
                    if ( !(x%density) )
                    {
                        switch( mFormat )
                        {
                            case FMT_GREY16 :
                                *(p+1) = 0;
                            case FMT_GREY :
                                *p = colY;
                                break;
                            case FMT_RGB :
                                *(p) = colR;
                                *(p+1) = colG;
                                *(p+2) = colB;
                                break;
                            case FMT_RGB48 :
                                *(p) = colR;
                                *(p+1) = 0;
                                *(p+2) = colG;
                                *(p+3) = 0;
                                *(p+4) = colB;
                                *(p+5) = 0;
                                break;
                            case FMT_YUV :
                            case FMT_YUVJ :
                                *(p) = colY;
                                *(p+1) = colU;
                                *(p+2) = colV;
                                break;
                            case FMT_YUVP :
                            case FMT_YUVJP :
                                *(p) = colY;
                                *(p+mPlaneSize) = colU;
                                *(p+(2*mPlaneSize)) = colV;
                                break;
                            case FMT_UNDEF :
                                break;
                        }
                    }
                }
            }
        }
        y++;
        for ( int i = numActiveEdges-1; i >= 0; i-- )
        {
            if ( y >= activeEdges[i].maxY ) // Or >= as per sheets
            {
                Debug( 9, "Deleting active_edge" );
                if ( i < (numActiveEdges-1) )
                {
                    memmove( &activeEdges[i], &activeEdges[i+1], sizeof(*activeEdges)*(numActiveEdges-i) );
                }
                numActiveEdges--;
            }
            else
            {
                activeEdges[i].minX += activeEdges[i]._1_m;
            }
        }
    } while ( numGlobalEdges || numActiveEdges );
}

/**
* @brief 
*
* @param colour
* @param box
*/
void Image::outline( Rgb colour, const Box &box )
{
    unsigned char colR = RGB_RED_VAL(colour);
    unsigned char colG = RGB_GREEN_VAL(colour);
    unsigned char colB = RGB_BLUE_VAL(colour);

    RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
    unsigned char colY = convFuncs.Y(colR,colG,colB);
    unsigned char colU = convFuncs.U(colR,colG,colB);
    unsigned char colV = convFuncs.V(colR,colG,colB);

    int loX = box.lo().x();
    int loY = box.lo().y();
    int hiX = box.hi().x();
    int hiY = box.hi().y();

    if ( mFormat == FMT_GREY || mFormat == FMT_GREY16 || mFormat == FMT_YUVJ || mFormat == FMT_YUVJP )
    {
        if ( colour == RGB_WHITE )
            colY = WHITE;
        else if ( colour == RGB_BLACK )
            colY = BLACK;
    }

    for ( int y = loY; y <= hiY; y++ )
    {
        unsigned char *p = mBuffer.data()+(((y*mWidth)+loX)*mPixelStep);
        for ( int x = loX; x <= hiX; )
        {
            switch( mFormat )
            {
                case FMT_GREY16 :
                    p[1] = 0;
                case FMT_GREY :
                    p[0] = colY;
                    break;
                case FMT_RGB :
                    p[0] = colR;
                    p[1] = colG;
                    p[2] = colB;
                    break;
                case FMT_RGB48 :
                    p[0] = colR;
                    p[1] = 0;
                    p[2] = colG;
                    p[3] = 0;
                    p[4] = colB;
                    p[5] = 0;
                    break;
                case FMT_YUV :
                case FMT_YUVJ :
                    p[0] = colY;
                    p[1] = colU;
                    p[2] = colV;
                    break;
                case FMT_YUVP :
                case FMT_YUVJP :
                    p[0] = colY;
                    p[mPlaneSize] = colU;
                    p[2*mPlaneSize] = colV;
                    break;
                case FMT_UNDEF :
                    break;
            }
            if ( y != loY && y != hiY )
            {
                x += box.width()-1;
                p += box.width() * mPixelStep;
            }
            else
            {
                x++;
                p += mPixelStep;
            }
        }
    }
}

/**
* @brief 
*
* @param colour
* @param polygon
*/
void Image::outline( Rgb colour, const Polygon &polygon )
{
    unsigned char colR = RGB_RED_VAL(colour);
    unsigned char colG = RGB_GREEN_VAL(colour);
    unsigned char colB = RGB_BLUE_VAL(colour);

    RGB2YUV &convFuncs = (mColourSpace==CS_YUV)?smRGB2YUV:smRGB2YUVJ;
    unsigned char colY = convFuncs.Y(colR,colG,colB);
    unsigned char colU = convFuncs.U(colR,colG,colB);
    unsigned char colV = convFuncs.V(colR,colG,colB);

    if ( mFormat == FMT_GREY || mFormat == FMT_GREY16 || mFormat == FMT_YUVJ || mFormat == FMT_YUVJP )
    {
        if ( colour == RGB_WHITE )
            colY = WHITE;
        else if ( colour == RGB_BLACK )
            colY = BLACK;
    }

    unsigned char *p = mBuffer.data();

    int numCoords = polygon.numCoords();
    for ( int j = 0, i = numCoords-1; j < numCoords; i = j++ )
    {
        const Coord &p1 = polygon.coord( i );
        const Coord &p2 = polygon.coord( j );

        int x1 = p1.x();
        int x2 = p2.x();
        int y1 = p1.y();
        int y2 = p2.y();

        double dx = x2 - x1;
        double dy = y2 - y1;

        double grad;

        if ( fabs(dx) <= fabs(dy) )
        {
            if ( y1 != y2 )
                grad = dx/dy;
            else
                grad = mWidth;

            double x;
            int y, yinc = (y1<y2)?1:-1;
            grad *= yinc;
            for ( x = x1, y = y1; y != y2; y += yinc, x += grad )
            {
                int offset = (y*mStride)+int(round(x)*mPixelStep);
                switch( mFormat )
                {
                    case FMT_GREY16 :
                        p[offset+1] = 0;
                    case FMT_GREY :
                        p[offset] = colY;
                        break;
                    case FMT_RGB :
                        p[offset] = colR;
                        p[offset+1] = colG;
                        p[offset+2] = colB;
                        break;
                    case FMT_RGB48 :
                        p[offset] = colR;
                        p[offset+1] = 0;
                        p[offset+2] = colG;
                        p[offset+3] = 0;
                        p[offset+4] = colB;
                        p[offset+5] = 0;
                        break;
                    case FMT_YUV :
                    case FMT_YUVJ :
                        p[offset] = colY;
                        p[offset+1] = colU;
                        p[offset+2] = colV;
                        break;
                    case FMT_YUVP :
                    case FMT_YUVJP :
                        p[offset] = colY;
                        p[offset+mPlaneSize] = colU;
                        p[offset+(2*mPlaneSize)] = colV;
                        break;
                    case FMT_UNDEF :
                        break;
                }
            }
        }
        else
        {
            if ( x1 != x2 )
                grad = dy/dx;
            else
                grad = mHeight;

            double y;
            int x, xinc = (x1<x2)?1:-1;
            grad *= xinc;
            for ( y = y1, x = x1; x != x2; x += xinc, y += grad )
            {
                int offset = (int(round(y))*mStride)+(x*mPixelStep);
                switch( mFormat )
                {
                    case FMT_GREY16 :
                        p[offset+1] = 0;
                    case FMT_GREY :
                        p[offset] = colY;
                        break;
                    case FMT_RGB :
                        p[offset] = colR;
                        p[offset+1] = colG;
                        p[offset+2] = colB;
                        break;
                    case FMT_RGB48 :
                        p[offset] = colR;
                        p[offset+1] = 0;
                        p[offset+2] = colG;
                        p[offset+3] = 0;
                        p[offset+4] = colB;
                        p[offset+5] = 0;
                        break;
                    case FMT_YUV :
                    case FMT_YUVJ :
                        p[offset] = colY;
                        p[offset+1] = colU;
                        p[offset+2] = colV;
                        break;
                    case FMT_YUVP :
                    case FMT_YUVJP :
                        p[offset] = colY;
                        p[offset+mPlaneSize] = colU;
                        p[offset+(2*mPlaneSize)] = colV;
                        break;
                    case FMT_UNDEF :
                        break;
                }
            }
        }
    }
}

/**
* @brief 
*
* @param angle
*/
void Image::rotate( int angle )
{
    angle %= 360;

    if ( !angle )
    {
        return;
    }
    if ( angle%90 )
    {
        Fatal( "Can't rotate %d degress, only cardinal angles, 90, 180, 270 degress supported", angle );
        return;
    }

    switch( angle )
    {
        case 90 :
        {
            Image rotateImage( mFormat, mHeight, mWidth );
            for ( int p = 0; p < mPlanes; p++ )
            {
                unsigned char *pSrc = mBuffer.data() + (p*mPlaneSize);
                for ( int x = rotateImage.mWidth-1; x >= 0; x-- )
                {
                    unsigned char *pDst = rotateImage.buffer().data() + (p*rotateImage.mStride*rotateImage.mHeight) + (x*mPixelStep);
                    for ( int y = 0; y < rotateImage.mHeight; y++ )
                    {
                        for ( int z = 0; z < mPixelStep; z++ )
                        {
                            *(pDst+z) = *pSrc++;
                        }
                        pDst += rotateImage.mStride;
                    }
                }
            }
            assign( rotateImage );
            break;
        }
        case 180 :
        {
            ByteBuffer rotateBuffer( mSize );
            for ( int p = 0; p < mPlanes; p++ )
            {
                unsigned char *pSrc = mBuffer.data() + (p*mPlaneSize);
                unsigned char *pDst = rotateBuffer.data() + ((p+1)*mPlaneSize) - mPixelStep;
                for ( int y = 0; y < mHeight; y++ )
                {
                    for ( int x = 0; x < mWidth; x++ )
                    {
                        for ( int z = 0; z < mPixelStep; z++ )
                        {
                            *(pDst+z) = *pSrc++;
                        }
                        pDst -= mPixelStep;
                    }
                }
            }
            mBuffer = rotateBuffer;
            break;
        }
        case 270 :
        {
            Image rotateImage( mFormat, mHeight, mWidth );
            for ( int p = 0; p < mPlanes; p++ )
            {
                unsigned char *pSrc = mBuffer.data() + (p*mPlaneSize);
                for ( int x = 0; x < rotateImage.mWidth; x++ )
                {
                    unsigned char *pDst = rotateImage.buffer().data() + (((p+1)*rotateImage.mStride*rotateImage.mHeight)-(rotateImage.mStride)) + (x*mPixelStep);
                    for ( int y = 0; y < rotateImage.mHeight; y++ )
                    {
                        for ( int z = 0; z < mPixelStep; z++ )
                        {
                            *(pDst+z) = *pSrc++;
                        }
                        pDst -= rotateImage.mStride;
                    }
                }
            }
            assign( rotateImage );
            break;
        }
    }
}

/**
* @brief 
*
* @param leftRight
*/
void Image::flip( bool leftRight )
{
    ByteBuffer flipBuffer( mBuffer.size() );
    if ( leftRight )
    {
        // Horizontal flip, left to right
        unsigned char *pSrc = mBuffer + mStride;
        unsigned char *pDst = flipBuffer;

        switch( mFormat )
        {
            case FMT_GREY :
            case FMT_GREY16 :
            case FMT_RGB :
            case FMT_RGB48 :
            case FMT_YUV :
            case FMT_YUVJ :
            {
                while( pSrc < mBuffer.tail() )
                {
                    for ( int x = 0; x < mWidth; x++ )
                    {
                        pSrc -= mPixelStep;
                        for ( int z = 0; z < mPixelStep; z++ )
                            *pDst++ = *(pSrc+z);
                    }
                    pSrc += 2*mStride;
                }
                break;
            }
            case FMT_YUVP :
            case FMT_YUVJP :
            {
                for ( int p = 0; p < mPlanes; p++ )
                {
                    unsigned char *pSrc = mBuffer + mStride + (p * mStride * mHeight);
                    unsigned char *pDst = flipBuffer + (p * mStride * mHeight);
                    unsigned char *pNextPlane = pDst + (mStride * mHeight);
                    while( pDst < pNextPlane )
                    {
                        for ( int j = 0; j < mWidth; j++ )
                        {
                            pSrc--;
                            *pDst++ = *pSrc;
                        }
                        pSrc += 2*mStride;
                    }
                }
                break;
            }
            default :
            {
                throw Exception( stringtf( "Unexpected format %d found when flipping image", mFormat ) );
                break;
            }
        }
    }
    else
    {
        // Vertical flip, top to bottom
        unsigned char *pSrc = mBuffer + (mHeight*mStride);
        unsigned char *pDst = flipBuffer;

        switch( mFormat )
        {
            case FMT_GREY :
            case FMT_GREY16 :
            case FMT_RGB :
            case FMT_RGB48 :
            case FMT_YUV :
            case FMT_YUVJ :
            {
                while( pSrc > mBuffer )
                {
                    pSrc -= mStride;
                    memcpy( pDst, pSrc, mStride );
                    pDst += mStride;
                }
                break;
            }
            case FMT_YUVP :
            case FMT_YUVJP :
            {
                for ( int p = 0; p < mPlanes; p++ )
                {
                    unsigned char *pNextPlane = pDst + (mStride * mHeight);
                    while( pDst < pNextPlane )
                    {
                        pSrc -= mStride;
                        memcpy( pDst, pSrc, mStride );
                        pDst += mStride;
                    }
                    pSrc += 2*(mHeight*mStride);
                    pDst = pNextPlane;
                }
                break;
            }
            default :
            {
                throw Exception( stringtf( "Unexpected format %d found when flipping image", mFormat ) );
                break;
            }
        }
    }
    mBuffer = flipBuffer;
}

/**
* @brief 
*
* @param factor
*/
void Image::scale( unsigned int factor )
{
    if ( !factor )
    {
        Error( "Bogus scale factor %d found", factor );
        return;
    }
    if ( factor == SCALE_BASE )
    {
        return;
    }

    Image scaleImage( mFormat, (int)round((1.0L*mWidth*factor)/SCALE_BASE), (int)round((1.0L*mHeight*factor)/SCALE_BASE) );
    if ( factor > SCALE_BASE )
    {
        unsigned char *pDst = scaleImage.mBuffer.data();
        unsigned int hCount = SCALE_BASE/2;
        unsigned int hLastIndex = 0;
        unsigned int hIndex;
        for ( int y = 0; y < mHeight; y++ )
        {
            unsigned char *pSrc = mBuffer.data() + (y*mStride);
            unsigned int wCount = SCALE_BASE/2;
            unsigned int wLastIndex = 0;
            unsigned int wIndex;
            for ( int x = 0; x < mWidth; x++ )
            {
                wCount += factor;
                wIndex = wCount/SCALE_BASE;
                for ( int f = wLastIndex; f < wIndex; f++ )
                {
                    for ( int p = 0; p < mPlanes; p++ )
                    {
                        for ( int c = 0; c < mPixelStep; c++ )
                        {
                            *(pDst+(p*scaleImage.mPlaneSize)+c) = *(pSrc+(p*mPlaneSize)+c);
                        }
                    }
                    pDst += scaleImage.mPixelStep;
                }
                pSrc += mPixelStep;
                wLastIndex = wIndex;
            }
            hCount += factor;
            hIndex = hCount/SCALE_BASE;
            for ( int f = hLastIndex+1; f < hIndex; f++ )
            {
                for ( int p = 0; p < mPlanes; p++ )
                {
                    memcpy( pDst+(p*scaleImage.mPlaneSize), (pDst-scaleImage.mStride)+(p*scaleImage.mPlaneSize), scaleImage.mStride );
                }
                pDst += scaleImage.mStride;
            }
            hLastIndex = hIndex;
        }
    }
    else
    {
        unsigned char *pDst = scaleImage.mBuffer.data();
        unsigned int xStart = factor/2;
        unsigned int yStart = factor/2;
        unsigned int hCount = yStart;
        unsigned int hLastIndex = 0;
        unsigned int hIndex;
        for ( unsigned int y = 0; y < mHeight; y++ )
        {
            hCount += factor;
            hIndex = hCount/SCALE_BASE;
            if ( hIndex > hLastIndex )
            {
                unsigned int wCount = xStart;
                unsigned int wLastIndex = 0;
                unsigned int wIndex;

                unsigned char *pSrc = mBuffer.data() + (y*mStride);
                for ( unsigned int x = 0; x < mWidth; x++ )
                {
                    wCount += factor;
                    wIndex = wCount/SCALE_BASE;

                    if ( wIndex > wLastIndex )
                    {
                        for ( int p = 0; p < mPlanes; p++ )
                        {
                            for ( int c = 0; c < mPixelStep; c++ )
                            {
                                *(pDst+(p*scaleImage.mPlaneSize)+c) = *(pSrc+(p*mPlaneSize)+c);
                            }
                        }
                        pSrc += mPixelStep;
                        pDst += scaleImage.mPixelStep;
                    }
                    else
                    {
                        pSrc += mPixelStep;
                    }
                    wLastIndex = wIndex;
                }
            }
            hLastIndex = hIndex;
        }
    }
    assign( scaleImage );
}

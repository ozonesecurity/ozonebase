#ifndef LIBIMG_IMAGE_H
#define LIBIMG_IMAGE_H

#include "config.h"
extern "C"
{
#include "libimgJpeg.h"
}
#include "libimgRgb.h"
#include "libimgCoord.h"
#include "libimgBox.h"
#include "libimgPoly.h"
#include "../libgen/libgenBuffer.h"
#include "../libgen/libgenThread.h"

#if HAVE_ZLIB_H
#include <zlib.h>
#endif // HAVE_ZLIB_H

#include <string>
#include <vector>

extern "C"
{
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

inline int clamp( int value, int min, int max )
{
    return( value<min?min:(value>max?max:value) );
}

class Image;

class Mask
{
protected:
    struct Range
    {
        int loX;
        int hiX;
        int offX;
    };

protected:
    Range   *mRanges;
    Image   *mMaskImage;

protected:
    Mask() : mRanges( 0 ), mMaskImage( 0 )
    {
    }
    virtual void createMaskImage() = 0;

public:
    virtual ~Mask();
    virtual int loX() const=0;
    virtual int hiX() const=0;
    virtual int loY() const=0;
    virtual int hiY() const=0;
    virtual int loX( int y ) const
    {
        return( mRanges[y].loX );
    }
    virtual int hiX( int y ) const
    {
        return( mRanges[y].hiX );
    }
    int width() const
    {
        return( extent().width() );
    }
    int height() const
    {
        return( extent().height() );
    }
    virtual const Box &extent() const=0;
    const Image *image() const;
    virtual unsigned long pixels() const=0;

    virtual void shrink( unsigned char factor ) = 0;
    virtual void swell( unsigned char factor ) = 0;
};

class BoxMask : public Mask
{
protected:
    Box mBox;

protected:
    void createMaskImage();

public:
    BoxMask( const Box &box );
    BoxMask( const BoxMask &mask );

    int loX() const { return( mBox.loX() ); }
    int hiX() const { return( mBox.hiX() ); }
    int loY() const { return( mBox.loY() ); }
    int hiY() const { return( mBox.hiY() ); }
    int loX( int y ) const { return( loX() ); }
    int hiX( int y ) const { return( hiX() ); }
    const Box &extent() const { return( mBox ); }
    unsigned long pixels() const;

    void shrink( unsigned char factor );
    void swell( unsigned char factor );

    BoxMask &operator=( const BoxMask &mask );
};

class PolyMask : public Mask
{
protected:
    Polygon mPoly;

protected:
    void createMaskImage();

public:
    PolyMask( const Polygon &poly );
    PolyMask( const PolyMask &mask );

    int loX() const { return( mPoly.loX() ); }
    int hiX() const { return( mPoly.hiX() ); }
    int loY() const { return( mPoly.loY() ); }
    int hiY() const { return( mPoly.hiY() ); }
    const Box &extent() const { return( mPoly.extent() ); }
    unsigned long pixels() const;

    void shrink( unsigned char factor );
    void swell( unsigned char factor );

    PolyMask &operator=( const PolyMask &mask );
};

class ImageMask : public Mask
{
private:
    Box mBox; // Extents only
    unsigned long mArea;

protected:
    void createMaskImage() { }

public:
    ImageMask( const Image &image );
    ImageMask( const ImageMask &mask );

    int loX() const { return( mBox.loX() ); }
    int hiX() const { return( mBox.hiX() ); }
    int loY() const { return( mBox.loY() ); }
    int hiY() const { return( mBox.hiY() ); }
    const Box &extent() const { return( mBox ); }
    unsigned long pixels() const;

    void shrink( unsigned char factor );
    void swell( unsigned char factor );

    ImageMask &operator=( const ImageMask &mask );
};

//
// This is image class, and represents a frame captured from a
// camera in raw form.
//
class Image
{
public:
    static const int MAX_IMAGE_WIDTH    = 2048;             // The largest image we imagine ever handling
    static const int MAX_IMAGE_HEIGHT   = 1536;             // The largest image we imagine ever handling
    static const int MAX_IMAGE_COLOURS  = 3;                // The largest image we imagine ever handling
    static const int MAX_IMAGE_DIM      = (MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT);
    static const int MAX_IMAGE_SIZE     = (MAX_IMAGE_DIM*MAX_IMAGE_COLOURS);

    static const int SCALE_BASE         = 100;

    static const int JPEG_QUALITY       = 70;               // Default jpeg quality, unless overridden
    static const bool ADD_JPEG_COMMENTS = true;             // Whether to include any image text in the jpeg file header
    static const bool ALLOW_JPEG_GREY_COLOURSPACE = true;   // Whether greyscale jpeg files are written in that colourspace or converted to colour

protected:
    enum { Y_CHAN=0, U_CHAN=1, V_CHAN=2, A_CHAN=3 };

    typedef unsigned char BlendTable[256][256];
    typedef BlendTable *BlendTablePtr;

    struct Edge
    {
        int minY;
        int maxY;
        double minX;
        double _1_m;

        static int CompareYX( const void *p1, const void *p2 )
        {
            const Edge *e1 = (const Edge *)p1, *e2 = (const Edge *)p2;
            if ( e1->minY == e2->minY )
                return( int(e1->minX - e2->minX) );
            else
                return( int(e1->minY - e2->minY) );
        }
        static int CompareX( const void *p1, const void *p2 )
        {
            const Edge *e1 = (const Edge *)p1, *e2 = (const Edge *)p2;
            return( int(e1->minX - e2->minX) );
        }
    };

    struct Lookups
    {
        // YUV -> YUVJ
        unsigned char   *YUV2YUVJ_Y;
        unsigned char   *YUV2YUVJ_UV;

        // YUVJ -> YUV
        unsigned char   *YUVJ2YUV_Y;
        unsigned char   *YUVJ2YUV_UV;

        // RGB -> YUVJ
        signed short    *RGB2YUVJ_YR;
        signed short    *RGB2YUVJ_YG;
        signed short    *RGB2YUVJ_YB;
        signed short    *RGB2YUVJ_UR;
        signed short    *RGB2YUVJ_UG;
        signed short    *RGB2YUVJ_UB;
        signed short    *RGB2YUVJ_VR;
        signed short    *RGB2YUVJ_VG;
        signed short    *RGB2YUVJ_VB;

        // YUVJ -> RGB
        signed short    *YUVJ2RGB_RV;
        signed short    *YUVJ2RGB_GU;
        signed short    *YUVJ2RGB_GV;
        signed short    *YUVJ2RGB_BU;

        // RGB -> YUV
        signed short    *RGB2YUV_YR;
        signed short    *RGB2YUV_YG;
        signed short    *RGB2YUV_YB;
        signed short    *RGB2YUV_UR;
        signed short    *RGB2YUV_UG;
        signed short    *RGB2YUV_UB;
        signed short    *RGB2YUV_VR;
        signed short    *RGB2YUV_VG;
        signed short    *RGB2YUV_VB;

        // YUV -> RGB
        signed short    *YUV2RGB_Y;
        signed short    *YUV2RGB_RV;
        signed short    *YUV2RGB_GU;
        signed short    *YUV2RGB_GV;
        signed short    *YUV2RGB_BU;
    };

    struct RGB2YUV
    {
        typedef unsigned char (*ConvFunc)( unsigned char, unsigned char, unsigned char );

        ConvFunc Y;
        ConvFunc U;
        ConvFunc V;
    };

    struct YUV2RGB
    {
        typedef unsigned char (*ConvFunc)( unsigned char, unsigned char, unsigned char );

        ConvFunc R;
        ConvFunc G;
        ConvFunc B;
    };

public:
    enum { OZ_CHAR_HEIGHT=11, OZ_CHAR_WIDTH=6 };
    enum { LINE_HEIGHT=OZ_CHAR_HEIGHT+0 };

    typedef enum { FMT_UNDEF, /*FMT_BITMAP,*/ FMT_GREY, FMT_GREY16, /*FMT_HSV,*/ FMT_RGB, FMT_RGB48, FMT_YUV, FMT_YUVJ, FMT_YUVP, FMT_YUVJP } Format;
    typedef enum { CS_UNDEF, /*CS_BITMAP,*/ CS_GREY, CS_YUV, CS_YUVJ, /*CS_HSV,*/ CS_RGB } ColourSpace;

    struct BlobData
    {
        unsigned char tag;
        int pixels;
        int loX;
        int hiX;
        int loY;
        int hiY;
        int midX;
        int midY;

        BlobData() :
            tag( 0 ),
            pixels( 0 ),
            loX( 0 ),
            hiX( 0 ),
            loY( 0 ),
            hiY( 0 ),
            midX( 0 ),
            midY( 0 )
        {
        }
        BlobData( const BlobData &blob ) :
            tag( blob.tag ),
            pixels( blob.pixels ),
            loX( blob.loX ),
            hiX( blob.hiX ),
            loY( blob.loY ),
            hiY( blob.hiY ),
            midX( blob.midX ),
            midY( blob.midY )
        {
        }
        BlobData &operator=( const BlobData &blob )
        {
            tag = blob.tag;
            pixels = blob.pixels;
            loX = blob.loX;
            hiX = blob.hiX;
            loY = blob.loY;
            hiY = blob.hiY;
            midX = blob.midX;
            midY = blob.midY;
            return( *this );
        }
    };
    class Blob
    {
    friend class Image;

    private:
        ImageMask mMask;
        Coord mOrigin;
        Coord mCentre;
        uint32_t mPixels;

    public:
        Blob( const Image &image, const Coord &origin, const Coord &centre, int pixels ) :
            mMask( image ),
            mOrigin( origin ),
            mCentre( centre ),
            mPixels( pixels )
        {
        }
        Blob( const Blob &blob ) :
            mMask( blob.mMask ),
            mOrigin( blob.mOrigin ),
            mCentre( blob.mCentre ),
            mPixels( blob.mPixels )
        {
        }
        uint32_t pixels() const
        {
            return( mPixels );
        }
        const Coord &origin() const
        {
            return( mOrigin );
        }
        const Coord &centre() const
        {
            return( mCentre );
        }
        int loX() const
        {
            return( mOrigin.x() );
        }
        int loY() const
        {
            return( mOrigin.y() );
        }
        int hiX() const
        {
            return( mOrigin.x()+mMask.width() );
        }
        int hiY() const
        {
            return( mOrigin.x()+mMask.height() );
        }
    };
    class BlobGroup
    {
    friend class Image;

    private:
        typedef std::vector<const Blob *> BlobList;

    private:
        BlobList    mBlobList;
        uint32_t    mMinSize;   // Filter for smallest blob
        uint32_t    mMaxSize;   // Filter for largest blob
        uint32_t    mPixels;
        Box         mExtent;
        Coord       mCentre;

    public:
        BlobGroup()
        {
            mMinSize = 0;
            mMaxSize = 0;
            mPixels = 0;
        }
        BlobGroup( const BlobGroup &blobGroup ) :
            mMinSize( blobGroup.mMinSize ),
            mMaxSize( blobGroup.mMaxSize ),
            mPixels( blobGroup.mPixels ),
            mExtent( blobGroup.mExtent ),
            mCentre( blobGroup.mCentre )
        {
            for ( BlobList::const_iterator iter = blobGroup.mBlobList.begin(); iter != blobGroup.mBlobList.end(); iter++ )
                mBlobList.push_back( new Blob( *(*iter) ) );
        }
        ~BlobGroup()
        {
            for ( BlobList::iterator iter = mBlobList.begin(); iter != mBlobList.end(); iter++ )
                delete *iter;
            mBlobList.clear();
        }
        //void addBlob( const Blob * )
        //{
        //}
        const BlobList &blobList() const
        {
            return( mBlobList );
        }
        uint32_t minSize() const
        {
            return( mMinSize );
        }
        void minSize( uint32_t newMinSize )
        {
            mMinSize = newMinSize;
        }
        uint32_t maxSize() const
        {
            return( mMaxSize );
        }
        void maxSize( uint32_t newMaxSize )
        {
            mMaxSize = newMaxSize;
        }
        void setFilter( uint32_t minSize, uint32_t maxSize )
        {
            mMinSize = minSize;
            mMaxSize = maxSize;
        }
        uint32_t pixels() const
        {
            return( mPixels );
        }
        const Box &extent() const
        {
            return( mExtent );
        }
        const Coord &centre() const
        {
            return( mCentre );
        }
        BlobGroup &operator=( const BlobGroup &blobGroup )
        {
            mMinSize = blobGroup.mMinSize;
            mMaxSize = blobGroup.mMaxSize;
            mPixels = blobGroup.mPixels;
            mExtent = blobGroup.mExtent;
            mCentre = blobGroup.mCentre;
            for ( BlobList::iterator iter = mBlobList.begin(); iter != mBlobList.end(); iter++ )
                delete *iter;
            mBlobList.clear();
            for ( BlobList::const_iterator iter = blobGroup.mBlobList.begin(); iter != blobGroup.mBlobList.end(); iter++ )
                mBlobList.push_back( new Blob( *(*iter) ) );
            return( *this );
        }
        BlobGroup &operator+=( const BlobGroup &blobGroup )
        {
            //if ( mBlobList.empty() )
                //mBlobList = blobGroup.mBlobList;
            //else
                //std::copy( blobGroup.mBlobList.begin(), blobGroup.mBlobList.end(), mBlobList.rbegin() );
            for ( BlobList::const_iterator iter = blobGroup.mBlobList.begin(); iter != blobGroup.mBlobList.end(); iter++ )
                mBlobList.push_back( new Blob( *(*iter) ) );
            mMinSize = std::min( mMinSize, blobGroup.mMinSize );
            mMaxSize = std::max( mMaxSize, blobGroup.mMaxSize );
            mPixels += blobGroup.mPixels;
            mExtent += blobGroup.mExtent;
            mCentre = mExtent.centre();
            return( *this );
        }
        friend BlobGroup operator+( const BlobGroup &blobGroup1, const BlobGroup &blobGroup2 )
        {
            BlobGroup result( blobGroup1 );
            result += blobGroup2;
            return( result );
        }
    };

protected:
    static bool             smInitialised;

    static BlendTablePtr    smBlendTables[101];

    static Lookups          smLookups;

    static RGB2YUV          smRGB2YUV;
    static RGB2YUV          smRGB2YUVJ;
    static YUV2RGB          smYUV2RGB;
    static YUV2RGB          smYUVJ2RGB;

public:
    static jpeg_compress_struct     *smJpegCoCinfo[101];
    static jpeg_decompress_struct   *smJpegDeCinfo;
    static local_error_mgr          smJpegCoError;
    static local_error_mgr          smJpegDeError;
    static Mutex                    smJpegCoMutex;
    static Mutex                    smJpegDeMutex;

protected:
    static void initialise();
    static void copyYUVP2YUVP( unsigned char *pDst, const unsigned char *pSrc, int width, int height, int densityX, int densityY, bool expand=true );
    static void copyYUYV2RGB( unsigned char *pDst, const unsigned char *pSrc, int width, int height );
    static void copyUYVY2RGB( unsigned char *pDst, const unsigned char *pSrc, int width, int height );
    static void copyYUYV2YUVP( unsigned char *pDst, const unsigned char *pSrc, int width, int height );
    static void copyUYVY2YUVP( unsigned char *pDst, const unsigned char *pSrc, int width, int height );
    static void copyRGBP2RGB( unsigned char *pDst, const unsigned char *pSrc, int width, int height, int rBits, int gBits, int bBits );
    static void copyBGR2RGB( unsigned char *pDst, const unsigned char *pSrc, int width, int height );

    static BlendTablePtr getBlendTable( int );

public:
    // Range conversions from YUVJ to YUV
    static inline unsigned char YUVJ2YUV_Y( unsigned char y )
    {
        return( smLookups.YUVJ2YUV_Y[y] );
    }
    static inline unsigned char YUVJ2YUV_U( unsigned char u )
    {
        return( smLookups.YUVJ2YUV_UV[u] );
    }
    static inline unsigned char YUVJ2YUV_V( unsigned char v )
    {
        return( smLookups.YUVJ2YUV_UV[v] );
    }
    static inline unsigned char YUVJ2YUV_UV( unsigned char uv )
    {
        return( smLookups.YUVJ2YUV_UV[uv] );
    }

    // Range conversions from YUV to YUVJ
    static inline unsigned char YUV2YUVJ_Y( unsigned char y )
    {
        return( smLookups.YUV2YUVJ_Y[y] );
    }
    static inline unsigned char YUV2YUVJ_U( unsigned char u )
    {
        return( smLookups.YUV2YUVJ_UV[u] );
    }
    static inline unsigned char YUV2YUVJ_V( unsigned char v )
    {
        return( smLookups.YUV2YUVJ_UV[v] );
    }
    static inline unsigned char YUV2YUVJ_UV( unsigned char uv )
    {
        return( smLookups.YUV2YUVJ_UV[uv] );
    }

    // Conversions from YUVJ to/from RGB
    static inline unsigned char RGB2YUVJ_Y( unsigned char r, unsigned char g, unsigned char b )
    {
        return( clamp( smLookups.RGB2YUVJ_YR[r] + smLookups.RGB2YUVJ_YG[g] + smLookups.RGB2YUVJ_YB[b], 0, 255 ) );
    }
    static inline unsigned char RGB2YUVJ_U( unsigned char r, unsigned char g, unsigned char b )
    {
        return( 128 + clamp( smLookups.RGB2YUVJ_UR[r] + smLookups.RGB2YUVJ_UG[g] + smLookups.RGB2YUVJ_UB[b], -127, 127 ) );
    }
    static inline unsigned char RGB2YUVJ_V( unsigned char r, unsigned char g, unsigned char b )
    {
        return( 128 + clamp( smLookups.RGB2YUVJ_VR[r] + smLookups.RGB2YUVJ_VG[g] + smLookups.RGB2YUVJ_VB[b], -127, 127 ) );
    }

    static inline unsigned char YUVJ2RGB_R( unsigned char y, unsigned char u, unsigned char v )
    {
        return( clamp( y + smLookups.YUVJ2RGB_RV[v], 0, 255 ) );
    }
    static inline unsigned char YUVJ2RGB_G( unsigned char y, unsigned char u, unsigned char v )
    {
        return( clamp( y + smLookups.YUVJ2RGB_GU[u] + (int)smLookups.YUVJ2RGB_GV[v], 0, 255) );
    }
    static inline unsigned char YUVJ2RGB_B( unsigned char y, unsigned char u, unsigned char v )
    {
        return( clamp( y + smLookups.YUVJ2RGB_BU[u], 0, 255 ) );
    }

    // Conversions from YUV to/from RGB
    static inline unsigned char RGB2YUV_Y( unsigned char r, unsigned char g, unsigned char b )
    {
        return( clamp( 16 + smLookups.RGB2YUV_YR[r] + smLookups.RGB2YUV_YG[g] + smLookups.RGB2YUV_YB[b], 16, 235 ) );
    }
    static inline unsigned char RGB2YUV_U( unsigned char r, unsigned char g, unsigned char b )
    {
        return( 128 + clamp( smLookups.RGB2YUV_UR[r] + smLookups.RGB2YUV_UG[g] + smLookups.RGB2YUV_UB[b], -112, 112 ) );
    }
    static inline unsigned char RGB2YUV_V( unsigned char r, unsigned char g, unsigned char b )
    {
        return( 128 + clamp( smLookups.RGB2YUV_VR[r] + smLookups.RGB2YUV_VG[g] + smLookups.RGB2YUV_VB[b], -112, 112 ) );
    }

    static inline unsigned char YUV2RGB_R( unsigned char y, unsigned char u, unsigned char v )
    {
        return( clamp( smLookups.YUV2RGB_Y[y] + smLookups.YUV2RGB_RV[v], 0, 255 ) );
    }
    static inline unsigned char YUV2RGB_G( unsigned char y, unsigned char u, unsigned char v )
    {
        return( clamp( smLookups.YUV2RGB_Y[y] + smLookups.YUV2RGB_GU[u] + (int)smLookups.YUV2RGB_GV[v], 0, 255) );
    }
    static inline unsigned char YUV2RGB_B( unsigned char y, unsigned char u, unsigned char v )
    {
        return( clamp( smLookups.YUV2RGB_Y[y] + smLookups.YUV2RGB_BU[u], 0, 255 ) );
    }

public:
#ifdef HAVE_LINUX_VIDEODEV2_H
    static Format getFormatFromPalette( int palette );
#endif
    static AVPixelFormat getFfPixFormat( Format format );
    static Format getFormatFromPixelFormat( AVPixelFormat pixelFormat );
    static AVPixelFormat getNativePixelFormat( AVPixelFormat pixelFormat )
    {
        return( getFfPixFormat( getFormatFromPixelFormat( pixelFormat ) ) );
    }

protected:
    Format mFormat;
    ColourSpace mColourSpace;
    int mWidth;
    int mHeight;
    int mPixels;
    int mChannels;
    int mPlanes;
    int mColourDepth;
    int mPixelWidth;
    int mPixelStep;
    int mStride;
    int mPlaneSize;
    int mSize;

    ByteBuffer mBuffer;

    std::string mText;

    const Mask *mMask;

protected:
    static jpeg_decompress_struct *jpegDcInfo();
    static jpeg_compress_struct *jpegCcInfo( int quality );

public:
    Image();
    Image( const char *filename );
    Image( Format format, int width, int height, unsigned char *data=NULL, bool adoptData=false );
#ifdef HAVE_LINUX_VIDEODEV2_H
    Image( int v4lPalette, int width, int height, unsigned char *data=NULL );
#endif
    Image( AVPixelFormat ffFormat, int width, int height, unsigned char *data=NULL );
    Image( const Image &image );
    Image( Format format, const Image &image );
    ~Image();

public:
#ifdef HAVE_LINUX_VIDEODEV2_H
    static size_t calcBufferSize( int v4lPalette, int width, int height );
#endif
    static size_t calcBufferSize( AVPixelFormat pixFormat, int width, int height );

public:
    inline Format format() const { return( mFormat ); }
    inline ColourSpace colourSpace() const { return( mColourSpace ); }
    inline int width() const { return( mWidth ); }
    inline int height() const { return( mHeight ); }
    inline int pixels() const { return( mPixels ); }
    inline int channels() const { return( mChannels ); }
    inline int planes() const { return( mPlanes ); }
    inline int colourDepth() const { return( mColourDepth ); }
    inline int pixelWidth() const { return( mPixelWidth ); }
    inline int pixelStep() const { return( mPixelStep ); }
    inline int stride() const { return( mStride ); }
    inline int size() const { return( mSize ); }

    AVPixelFormat pixelFormat() const { return( getFfPixFormat( mFormat ) ); }

    const ByteBuffer &buffer() const { return( mBuffer ); }
    ByteBuffer &buffer() { return( mBuffer ); }
    const unsigned char *buffer( int y ) const { return( mBuffer.data()+(y*mStride) ); }
    unsigned char *buffer( int y ) { return( mBuffer.data()+(y*mStride) ); }
    const unsigned char *buffer( int x, int y ) const { return( mBuffer.data()+(y*mStride)+(x*mPixelStep) ); }
    unsigned char *buffer( int x, int y ) { return( mBuffer.data()+(y*mStride)+(x*mPixelStep) ); }

    unsigned char red( int p ) const;
    unsigned char green( int p ) const;
    unsigned char blue( int p ) const;
    unsigned char red( int x, int y ) const;
    unsigned char green( int x, int y ) const;
    unsigned char blue( int x, int y ) const;

    unsigned char y( int p ) const;
    unsigned char u( int p ) const;
    unsigned char v( int p ) const;
    unsigned char y( int x, int y ) const;
    unsigned char u( int x, int y ) const;
    unsigned char v( int x, int y ) const;

    bool hasMask() const { return( mMask != 0 ); }
    void setMask( const Mask *mask )
    {
        mMask = mask;
    }
    const Mask *getMask() const
    {
        return( mMask );
    }

    void dump( int lines=0, int cols=0 ) const;

    void clear();
    bool empty() { return( mSize == 0 ); }
    void copy( const Image &image );
    void assign( const Image &image );
    void assign( Format format, int width, int height, unsigned char *data=NULL, bool adoptData=false );
    void convert( const Image &image );

    bool readRaw( const char *filename );
    bool writeRaw( const char *filename ) const;

    bool readJpeg( const std::string &filename );
    bool writeJpeg( const std::string &filename, int quality_override=0 ) const;
    bool decodeJpeg( const unsigned char *inbuffer, int inbufferSize );
    bool encodeJpeg( unsigned char *outbuffer, int *outbufferSize, int quality_override=0 ) const;

#if HAVE_ZLIB_H
    bool unzip( const Bytef *inbuffer, unsigned long inbufferSize );
    bool zip( Bytef *outbuffer, unsigned long *outbufferSize, int compression_level=Z_BEST_SPEED ) const;
#endif // HAVE_ZLIB_H

    inline int *rgbBuffer( unsigned int x, unsigned int y ) const;
    inline int *yChannel( unsigned int x, unsigned int y ) const;

    bool crop( int loX, int loY, int hiX, int hiY );
    bool crop( const Box &limits );

    void overlay( const Image &image );
    void overlay( const Image &image, int x, int y );
    void blend( const Image &image, unsigned short fraction ) const;
    void decay( unsigned short fraction ) const;
    void boost( unsigned short fraction ) const;
    void square() const;
    void squareRoot() const;
    static Image *merge( int n_images, Image *images[] );
    static Image *merge( int n_images, Image *images[], double weight );
    static Image *highlight( int n_images, Image *images[], const Rgb threshold=RGB_BLACK, const Rgb ref_colour=RGB_RED );
    Image *delta( const Image &image, bool correct=false ) const;
    Image *delta2( const Image &image, bool correct=false ) const;
    Image *yDelta( const Image &image, bool correct=false ) const;
    Image *yDelta2( const Image &image, bool correct=false ) const;
    unsigned long long total() const;
    unsigned long long total( int channel ) const;
    void despeckleFilter( int window );
    void medianFilter( int window );
    int locateBlobs( BlobGroup &blobData, const Mask *mask, bool weightBlobCentres=true, bool tidyBlobs=false );

    const Coord textCentreCoord( const char *text );
    void annotate( const char *text, const Coord &coord,  const Rgb fg_colour=RGB_WHITE, const Rgb bg_colour=RGB_BLACK );
    Image *highlightEdges( Rgb colour, const Mask *mask ) const;
    Image *highlightEdges( Rgb colour ) const;
    //Image *highlightEdges( Rgb colour, const Polygon &polygon );
    void timestamp( const char *label, const time_t when, const Coord &coord );

    void erase();
    void fill( Rgb colour, unsigned char density=1 );
    void fill( Rgb colour, const Coord &point );
    void fill( Rgb colour, const Box &limits, unsigned char density=1 );
    void fill( Rgb colour, const Polygon &polygon, unsigned char density=1 );

    void outline( Rgb colour, const Box &box );
    void outline( Rgb colour, const Polygon &polygon );

    void rotate( int angle );
    void flip( bool leftright );

    void scale( unsigned int factor );
    void shrink( unsigned char factor );
    void swell( unsigned char factor );

    Image &operator=( const Image &image )
    {
        convert( image );
        return( *this );
    }
};

#endif // LIBIMG_IMAGE_H

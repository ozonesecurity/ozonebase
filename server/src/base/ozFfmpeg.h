#ifndef OZ_FFMPEG_H
#define OZ_FFMPEG_H

#include "../libgen/libgenThread.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#ifdef __cplusplus
}
#endif

AVRational double2Rational( double input, long maxden );

class Rational : public AVRational
{
public:
    Rational()
    {
        num = 0;
        den = 0;
    }
    Rational( int numerator, int denominator )
    {
        num = numerator;
        den = denominator;
    }
    Rational( int numerator )
    {
        num = numerator;
        den = 1;
    }
    Rational( double rate )
    {
        *this = double2Rational( rate, 10000 );
    }
    Rational( AVRational rate )
    {
        num = rate.num;
        den = rate.den;
    }

    operator int() const { return( toInt() ); }
    operator double() const { return( toDouble() ); }
    friend int operator*( int value, const Rational &rational ) { return( (value*rational.num)/rational.den ); }
    int toInt() const { return( num/den ); }
    double toDouble() const { return( (double)num/den ); }
    Rational invert() const { return( Rational( den, num ) ); }
};

class TimeBase;
class FrameRate : public Rational
{
public:
    FrameRate()
    {
    }
    FrameRate( int numerator, int denominator ) :
        Rational( numerator, denominator )
    {
    }
    FrameRate( int numerator ) :
        Rational( numerator )
    {
    }
    FrameRate( double rate ) :
        Rational( rate )
    {
    }
    FrameRate( AVRational rate ) :
        Rational( rate )
    {
    }
    TimeBase timeBase() const;
    double interval() const
    {
        return( (double)den/num );
    }
    uint64_t intervalUsec() const
    {
        return( ((uint64_t)den*1000000LL)/num );
    }
    uint64_t intervalPTS( const TimeBase &timeBase ) const;
};

class TimeBase : public Rational
{
public:
    TimeBase()
    {
    }
    TimeBase( int numerator, int denominator ) :
        Rational( numerator, denominator )
    {
    }
    TimeBase( int numerator ) :
        Rational( numerator )
    {
    }
    TimeBase( double base ) :
        Rational( base )
    {
    }
    TimeBase( AVRational base ) :
        Rational( base )
    {
    }
    FrameRate frameRate() const
    {
        return( invert() );
    }
};

inline TimeBase FrameRate::timeBase() const
{
    return( invert() );
}

inline uint64_t FrameRate::intervalPTS( const TimeBase &timeBase ) const
{
    return( (den*timeBase.den)/(num*timeBase.num) );
}

/* NAL unit types */
typedef enum {
    NAL_SLICE=1,
    NAL_DPA,
    NAL_DPB,
    NAL_DPC,
    NAL_IDR_SLICE,
    NAL_SEI,
    NAL_SPS,
    NAL_PPS,
    NAL_AUD,
    NAL_END_SEQUENCE,
    NAL_END_STREAM,
    NAL_FILLER_DATA,
    NAL_SPS_EXT,
    NAL_AUXILIARY_SLICE=19
} H264NalType;

const uint8_t *h264StartCode( const uint8_t *p, const uint8_t *end );

void avInit();
const char *avStrError( int error );
void avDumpDict( AVDictionary *dict );
void avDictSet( AVDictionary **dict, const char *name, const char *value );
void avDictSet( AVDictionary **dict, const char *name, int value );
void avDictSet( AVDictionary **dict, const char *name, double value );
void avSetH264Profile( AVDictionary **dict, const std::string &profile );
void avSetH264Preset( AVDictionary **dict, const std::string &preset );

enum AVPixelFormat choose_pixel_fmt(AVStream *st, AVCodecContext *enc_ctx, AVCodec *codec, enum AVPixelFormat target);
void choose_sample_fmt(AVStream *st, AVCodec *codec);

#endif // OZ_FFMPEG_H

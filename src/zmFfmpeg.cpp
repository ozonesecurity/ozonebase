#include "zm.h"
#include "zmFfmpeg.h"

#include "libgen/libgenDebug.h"
#include "libgen/libgenThread.h"

/*
** find rational approximation to given real number
** David Eppstein / UC Irvine / 8 Aug 1993
**
** With corrections from Arno Formella, May 2008
**
** http://www.ics.uci.edu/~eppstein/numth/frap.c
**/

AVRational double2Rational( double input, long maxden )
{
    //printf( "%.3lf\n", input );
    double startx, x;
    startx = x = input;

    long m[2][2];
    /* initialize matrix */
    m[0][0] = m[1][1] = 1;
    m[0][1] = m[1][0] = 0;

    int loop = 0;
    long ai;
    /* loop finding terms until denom gets too big */
    while ( m[1][0] * ( ai = (long)x ) + m[1][1] <= maxden )
    {
        long t;
        t = m[0][0] * ai + m[0][1];
        m[0][1] = m[0][0];
        m[0][0] = t;
        t = m[1][0] * ai + m[1][1];
        m[1][1] = m[1][0];
        m[1][0] = t;
        if ( x == (double)ai )
        {
            //printf( "div/zero\n" );
            break;     // AF: division by zero
        }
        x = 1/(x - (double) ai);
        if ( x > (double)0x7FFFFFFF )
        {
            //printf( "rep fail\n" );
            break;  // AF: representation failure
        }
        loop++;
    } 
    //printf( "L:%d\n", loop );
    if ( loop == 0 )
        return( (AVRational){ (int)input, 1 } );

    /* now remaining x is between 0 and 1/ai */
    /* approx as either 0 or 1/m where m is max that will fit in maxden */
    /* first try zero */
    //printf( "%ld/%ld, error = %e\n", m[0][0], m[1][0], startx - ((double) m[0][0] / (double) m[1][0]) );

    //double error1 = startx - ((double) m[0][0] / (double) m[1][0]);
    AVRational answer1 = { m[0][0], m[1][0] };

#if 0
    /* now try other possibility */
    ai = (maxden - m[1][1]) / m[1][0];
    m[0][0] = m[0][0] * ai + m[0][1];
    m[1][0] = m[1][0] * ai + m[1][1];
    printf("%ld/%ld, error = %e\n", m[0][0], m[1][0], startx - ((double) m[0][0] / (double) m[1][0]));

    double error2 = startx - ((double) m[0][0] / (double) m[1][0]);
    AVRational answer2 = { m[0][0], m[1][0] };

    if ( error1 < error2 )
        printf( "A1\n" );
    else
        printf( "A2\n" );

    if ( error1 > error2 )
        return( answer2 );
#endif
    return( answer1 );
}

static int ffmpegLockManager( void **mutex, enum AVLockOp op ) 
{ 
    //pthread_mutex_t** pmutex = (pthread_mutex_t**) mutex;
    Mutex **ffmpegMutex = (Mutex **)mutex;
    switch (op)
    {
        case AV_LOCK_CREATE: 
            //*pmutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)); 
            //pthread_mutex_init(*pmutex, NULL);
            *ffmpegMutex = new Mutex;
            break;
        case AV_LOCK_OBTAIN:
            //pthread_mutex_lock(*pmutex);
            (*ffmpegMutex)->lock();
            break;
        case AV_LOCK_RELEASE:
            //pthread_mutex_unlock(*pmutex);
            (*ffmpegMutex)->unlock();
            break;
        case AV_LOCK_DESTROY:
            //pthread_mutex_destroy(*pmutex);
            //free(*pmutex);
            delete *ffmpegMutex;
            break;
    }
    return 0;
}

void ffmpegInit()
{
    av_log_set_level( ( dbgLevel > DBG_INF ) ? AV_LOG_DEBUG : AV_LOG_QUIET );

    av_lockmgr_register( ffmpegLockManager );

    avcodec_register_all();
    avdevice_register_all();
    av_register_all();
    avformat_network_init();
}

const char *avStrError( int error )
{
    static char avErrorBuffer[256] = ""; // Should be mutex protected really
    if ( av_strerror( error, avErrorBuffer, sizeof(avErrorBuffer) ) != 0 )
        Panic( "Can't find string for AVERROR %d", error );
    return( avErrorBuffer );
}

void avDumpDict( AVDictionary *dict )
{
    Info( "Dictionary" );
    const AVDictionaryEntry *entry = NULL;
    while( (entry = av_dict_get( dict, "", entry, AV_DICT_IGNORE_SUFFIX )) )
    {
        Info( "Dict: %s = %s", entry->key, entry->value );
    }
}

void avDictSet( AVDictionary **dict, const char *name, const char *value )
{
    int avError = av_dict_set( dict, name, value, 0 );
    if ( avError != 0 )
    {
        Error( "Unable to set dictionary value '%s' to '%s'", name, value );
        avStrError( avError );
    }
}

void avDictSet( AVDictionary **dict, const char *name, int value )
{
    char valueString[64] = "";
    snprintf( valueString, sizeof(valueString), "%d", value );
    avDictSet( dict, name, valueString );
}

void avDictSet( AVDictionary **dict, const char *name, double value )
{
    char valueString[64] = "";
    snprintf( valueString, sizeof(valueString), "%.2f", value );
    avDictSet( dict, name, valueString );
}

///< These could possibly read the presets from the ffmpeg files themselves

/// This must be called _after_ preset function below to have any effect
void avSetH264Profile( AVDictionary **dict, const std::string &profile )
{
    if ( profile == "baseline" )
    {
        // x264 Baseline
        av_dict_set( dict, "coder", "0", 0 );
        av_dict_set( dict, "bf", "0", 0 );
        av_dict_set( dict, "flags2", "-wpred-dct8x8", AV_DICT_APPEND );
        av_dict_set( dict, "wpredp", "0", 0 );
    }
    else if ( profile == "main" )
    {
        av_dict_set( dict, "flags2", "-dct8x8", AV_DICT_APPEND );
    }
    else if ( profile == "high" )
    {
        // No limitations
    }
    else
    {
        Fatal( "Unexpected H.264 profile '%s'", profile.c_str() );
    }
}

void avSetH264Preset( AVDictionary **dict, const std::string &preset )
{
    if ( preset == "default" )
    {
        // x264 Default
        av_dict_set( dict, "coder", "1", 0 );
        av_dict_set( dict, "flags", "+loop", 0 );
        av_dict_set( dict, "cmp", "+chroma", 0 );
        av_dict_set( dict, "partitions", "+parti8x8+parti4x4+partp8x8+partb8x8", 0 );
        av_dict_set( dict, "me_method", "hex", 0 );
        av_dict_set( dict, "subq", "7", 0 );
        av_dict_set( dict, "me_range", "16", 0 );
        av_dict_set( dict, "g", "250", 0 );
        av_dict_set( dict, "keyint_min", "25", 0 );
        av_dict_set( dict, "sc_threshold", "40", 0 );
        av_dict_set( dict, "i_qfactor", "0.71", 0 );
        av_dict_set( dict, "b_strategy", "1", 0 );
        av_dict_set( dict, "qcomp", "0.6", 0 );
        av_dict_set( dict, "qmin", "10", 0 );
        av_dict_set( dict, "qmax", "51", 0 );
        av_dict_set( dict, "qdiff", "4", 0 );
        av_dict_set( dict, "bf", "3", 0 );
        av_dict_set( dict, "refs", "3", 0 );
        av_dict_set( dict, "directpred", "1", 0 );
        av_dict_set( dict, "trellis", "1", 0 );
        av_dict_set( dict, "flags2", "+mixed_refs+wpred+dct8x8+fastpskip", 0 );
        av_dict_set( dict, "wpredp", "2", 0 );
    }
    else if ( preset == "normal" )
    {
        // x264 Normal
        av_dict_set( dict, "coder", "1", 0 );
        av_dict_set( dict, "flags", "+loop", 0 );
        av_dict_set( dict, "cmp", "+chroma", 0 );
        av_dict_set( dict, "partitions", "+parti8x8+parti4x4+partp8x8+partb8x8", 0 );
        av_dict_set( dict, "me_method", "hex", 0 );
        av_dict_set( dict, "subq", "6", 0 );
        av_dict_set( dict, "me_range", "16", 0 );
        av_dict_set( dict, "g", "250", 0 );
        av_dict_set( dict, "keyint_min", "25", 0 );
        av_dict_set( dict, "sc_threshold", "40", 0 );
        av_dict_set( dict, "i_qfactor", "0.71", 0 );
        av_dict_set( dict, "b_strategy", "1", 0 );
        av_dict_set( dict, "qcomp", "0.6", 0 );
        av_dict_set( dict, "qmin", "10", 0 );
        av_dict_set( dict, "qmax", "51", 0 );
        av_dict_set( dict, "qdiff", "4", 0 );
        av_dict_set( dict, "bf", "3", 0 );
        av_dict_set( dict, "refs", "2", 0 );
        av_dict_set( dict, "directpred", "3", 0 );
        av_dict_set( dict, "trellis", "0", 0 );
        av_dict_set( dict, "flags2", "+wpred+dct8x8+fastpskip", 0 );
        av_dict_set( dict, "wpredp", "2", 0 );
    }
    else if ( preset == "medium" )
    {
        // x264 Medium
        av_dict_set( dict, "coder", "1", 0 );
        av_dict_set( dict, "flags", "+loop", 0 );
        av_dict_set( dict, "cmp", "+chroma", 0 );
        av_dict_set( dict, "partitions", "+parti8x8+parti4x4+partp8x8+partb8x8", 0 );
        av_dict_set( dict, "me_method", "hex", 0 );
        av_dict_set( dict, "subq", "7", 0 );
        av_dict_set( dict, "me_range", "16", 0 );
        av_dict_set( dict, "g", "250", 0 );
        av_dict_set( dict, "keyint_min", "25", 0 );
        av_dict_set( dict, "sc_threshold", "40", 0 );
        av_dict_set( dict, "i_qfactor", "0.71", 0 );
        av_dict_set( dict, "b_strategy", "1", 0 );
        av_dict_set( dict, "qcomp", "0.6", 0 );
        av_dict_set( dict, "qmin", "10", 0 );
        av_dict_set( dict, "qmax", "51", 0 );
        av_dict_set( dict, "qdiff", "4", 0 );
        av_dict_set( dict, "bf", "3", 0 );
        av_dict_set( dict, "refs", "3", 0 );
        av_dict_set( dict, "directpred", "1", 0 );
        av_dict_set( dict, "trellis", "1", 0 );
        av_dict_set( dict, "flags2", "+bpyramid+mixed_refs+wpred+dct8x8+fastpskip", 0 );
        av_dict_set( dict, "wpredp", "2", 0 );
    }
    else if ( preset == "hq" )
    {
		av_dict_set( dict, "coder", "1", 0 );
		av_dict_set( dict, "flags", "+loop", 0 );
		av_dict_set( dict, "cmp", "+chroma", 0 );
		av_dict_set( dict, "partitions", "+parti8x8+parti4x4+partp8x8+partb8x8", 0 );
		av_dict_set( dict, "me_method", "umh", 0 );
		av_dict_set( dict, "subq", "8", 0 );
		av_dict_set( dict, "me_range", "16", 0 );
		av_dict_set( dict, "g", "250", 0 );
		av_dict_set( dict, "keyint_min", "25", 0 );
		av_dict_set( dict, "sc_threshold", "40", 0 );
		av_dict_set( dict, "i_qfactor", "0.71", 0 );
		av_dict_set( dict, "b_strategy", "2", 0 );
		av_dict_set( dict, "qcomp", "0.6", 0 );
		av_dict_set( dict, "qmin", "10", 0 );
		av_dict_set( dict, "qmax", "51", 0 );
		av_dict_set( dict, "qdiff", "4", 0 );
		av_dict_set( dict, "bf", "3", 0 );
		av_dict_set( dict, "refs", "4", 0 );
		av_dict_set( dict, "directpred", "3", 0 );
		av_dict_set( dict, "trellis", "1", 0 );
		av_dict_set( dict, "flags2", "+wpred+mixed_refs+dct8x8+fastpskip", 0 );
		av_dict_set( dict, "wpredp", "2", 0 );
    }
    else if ( preset == "fast" )
    {
		av_dict_set( dict, "coder", "1", 0 );
		av_dict_set( dict, "flags", "+loop", 0 );
		av_dict_set( dict, "cmp", "+chroma", 0 );
		av_dict_set( dict, "partitions", "+parti8x8+parti4x4+partp8x8+partb8x8", 0 );
		av_dict_set( dict, "me_method", "hex", 0 );
		av_dict_set( dict, "subq", "6", 0 );
		av_dict_set( dict, "me_range", "16", 0 );
		av_dict_set( dict, "g", "250", 0 );
		av_dict_set( dict, "keyint_min", "25", 0 );
		av_dict_set( dict, "sc_threshold", "40", 0 );
		av_dict_set( dict, "i_qfactor", "0.71", 0 );
		av_dict_set( dict, "b_strategy", "1", 0 );
		av_dict_set( dict, "qcomp", "0.6", 0 );
		av_dict_set( dict, "qmin", "10", 0 );
		av_dict_set( dict, "qmax", "51", 0 );
		av_dict_set( dict, "qdiff", "4", 0 );
		av_dict_set( dict, "bf", "3", 0 );
		av_dict_set( dict, "refs", "2", 0 );
		av_dict_set( dict, "directpred", "1", 0 );
		av_dict_set( dict, "trellis", "1", 0 );
		av_dict_set( dict, "flags2", "+bpyramid+mixed_refs+wpred+dct8x8+fastpskip", 0 );
		av_dict_set( dict, "wpredp", "2", 0 );
		av_dict_set( dict, "rc_lookahead", "30", 0 );
    }
    else if ( preset == "faster" )
    {
		av_dict_set( dict, "coder", "1", 0 );
		av_dict_set( dict, "flags", "+loop", 0 );
		av_dict_set( dict, "cmp", "+chroma", 0 );
		av_dict_set( dict, "partitions", "+parti8x8+parti4x4+partp8x8+partb8x8", 0 );
		av_dict_set( dict, "me_method", "hex", 0 );
		av_dict_set( dict, "subq", "4", 0 );
		av_dict_set( dict, "me_range", "16", 0 );
		av_dict_set( dict, "g", "250", 0 );
		av_dict_set( dict, "keyint_min", "25", 0 );
		av_dict_set( dict, "sc_threshold", "40", 0 );
		av_dict_set( dict, "i_qfactor", "0.71", 0 );
		av_dict_set( dict, "b_strategy", "1", 0 );
		av_dict_set( dict, "qcomp", "0.6", 0 );
		av_dict_set( dict, "qmin", "10", 0 );
		av_dict_set( dict, "qmax", "51", 0 );
		av_dict_set( dict, "qdiff", "4", 0 );
		av_dict_set( dict, "bf", "3", 0 );
		av_dict_set( dict, "refs", "2", 0 );
		av_dict_set( dict, "directpred", "1", 0 );
		av_dict_set( dict, "trellis", "1", 0 );
		av_dict_set( dict, "flags2", "+bpyramid-mixed_refs+wpred+dct8x8+fastpskip", 0 );
		av_dict_set( dict, "wpredp", "1", 0 );
		av_dict_set( dict, "rc_lookahead", "20", 0 );
    }
    else if ( preset == "veryfast" )
    {
		av_dict_set( dict, "coder", "1", 0 );
		av_dict_set( dict, "flags", "+loop", 0 );
		av_dict_set( dict, "cmp", "+chroma", 0 );
		av_dict_set( dict, "partitions", "+parti8x8+parti4x4+partp8x8+partb8x8", 0 );
		av_dict_set( dict, "me_method", "hex", 0 );
		av_dict_set( dict, "subq", "2", 0 );
		av_dict_set( dict, "me_range", "16", 0 );
		av_dict_set( dict, "g", "250", 0 );
		av_dict_set( dict, "keyint_min", "25", 0 );
		av_dict_set( dict, "sc_threshold", "40", 0 );
		av_dict_set( dict, "i_qfactor", "0.71", 0 );
		av_dict_set( dict, "b_strategy", "1", 0 );
		av_dict_set( dict, "qcomp", "0.6", 0 );
		av_dict_set( dict, "qmin", "10", 0 );
		av_dict_set( dict, "qmax", "51", 0 );
		av_dict_set( dict, "qdiff", "4", 0 );
		av_dict_set( dict, "bf", "3", 0 );
		av_dict_set( dict, "refs", "1", 0 );
		av_dict_set( dict, "directpred", "1", 0 );
		av_dict_set( dict, "trellis", "0", 0 );
		av_dict_set( dict, "flags2", "+bpyramid-mixed_refs+wpred+dct8x8+fastpskip", 0 );
		av_dict_set( dict, "wpredp", "0", 0 );
		av_dict_set( dict, "rc_lookahead", "10", 0 );
    }
    else if ( preset == "superfast" )
    {
		av_dict_set( dict, "coder", "1", 0 );
		av_dict_set( dict, "flags", "+loop", 0 );
		av_dict_set( dict, "cmp", "+chroma", 0 );
		av_dict_set( dict, "partitions", "+parti8x8+parti4x4-partp8x8-partb8x8", 0 );
		av_dict_set( dict, "me_method", "dia", 0 );
		av_dict_set( dict, "subq", "1", 0 );
		av_dict_set( dict, "me_range", "16", 0 );
		av_dict_set( dict, "g", "250", 0 );
		av_dict_set( dict, "keyint_min", "25", 0 );
		av_dict_set( dict, "sc_threshold", "40", 0 );
		av_dict_set( dict, "i_qfactor", "0.71", 0 );
		av_dict_set( dict, "b_strategy", "1", 0 );
		av_dict_set( dict, "qcomp", "0.6", 0 );
		av_dict_set( dict, "qmin", "10", 0 );
		av_dict_set( dict, "qmax", "51", 0 );
		av_dict_set( dict, "qdiff", "4", 0 );
		av_dict_set( dict, "bf", "3", 0 );
		av_dict_set( dict, "refs", "1", 0 );
		av_dict_set( dict, "directpred", "1", 0 );
		av_dict_set( dict, "trellis", "0", 0 );
		av_dict_set( dict, "flags2", "+bpyramid-mixed_refs+wpred+dct8x8+fastpskip-mbtree", 0 );
		av_dict_set( dict, "wpredp", "0", 0 );
		av_dict_set( dict, "rc_lookahead", "0", 0 );
    }
    else if ( preset == "ultrafast" )
    {
		av_dict_set( dict, "coder", "0", 0 );
		av_dict_set( dict, "flags", "-loop", 0 );
		av_dict_set( dict, "cmp", "+chroma", 0 );
		av_dict_set( dict, "partitions", "-parti8x8-parti4x4-partp8x8-partb8x8", 0 );
		av_dict_set( dict, "me_method", "dia", 0 );
		av_dict_set( dict, "subq", "0", 0 );
		av_dict_set( dict, "me_range", "16", 0 );
		av_dict_set( dict, "g", "250", 0 );
		av_dict_set( dict, "keyint_min", "25", 0 );
		av_dict_set( dict, "sc_threshold", "0", 0 );
		av_dict_set( dict, "i_qfactor", "0.71", 0 );
		av_dict_set( dict, "b_strategy", "0", 0 );
		av_dict_set( dict, "qcomp", "0.6", 0 );
		av_dict_set( dict, "qmin", "10", 0 );
		av_dict_set( dict, "qmax", "51", 0 );
		av_dict_set( dict, "qdiff", "4", 0 );
		av_dict_set( dict, "bf", "0", 0 );
		av_dict_set( dict, "refs", "1", 0 );
		av_dict_set( dict, "directpred", "1", 0 );
		av_dict_set( dict, "trellis", "0", 0 );
		av_dict_set( dict, "flags2", "-bpyramid-mixed_refs-wpred-dct8x8+fastpskip-mbtree", 0 );
		av_dict_set( dict, "wpredp", "0", 0 );
		av_dict_set( dict, "aq_mode", "0", 0 );
		av_dict_set( dict, "rc_lookahead", "0", 0 );
    }
    else
    {
        Fatal( "Unexpected H.264 preset '%s'", preset.c_str() );
    }
}

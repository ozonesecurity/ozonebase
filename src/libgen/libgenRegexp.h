#ifndef LIBGEN_REGEXP_H
#define LIBGEN_REGEXP_H

#include "config.h"

#if HAVE_LIBPCRE

#if HAVE_PCRE_H
#include <pcre.h>
#elif HAVE_PCRE_PCRE_H
#include <pcre/pcre.h>
#else
#error Unable to locate pcre.h, please do 'locate pcre.h' and report location to zoneminder.com
#endif

class RegExpr
{
protected:
    pcre *mRegex;
    pcre_extra *mRegextra;
    int mMaxMatches;
    int *mMatchVectors;
    mutable char **mMatchBuffers;
    int *mMatchLengths;
    bool *mMatchValid;

protected:
    const char *mMatchString;
    int mNumMatches;
    
protected:
    bool mOk;

public:
    RegExpr( const char *pattern, int cflags=0, int maxMatches=32 );
    ~RegExpr();
    bool ok() const { return( mOk ); }
    int matchCount() const { return( mNumMatches ); }
    int match( const char *subjectString, int subjectLength, int flags=0 );
    const char *matchString( int matchIndex ) const;
    int matchLength( int matchIndex ) const;
};

#endif // HAVE_LIBPCRE

#endif // LIBGEN_REGEXP_H

#include "libgenRegexp.h"
#include "libgenDebug.h"

#include <string.h>

#if HAVE_LIBPCRE

RegExpr::RegExpr( const char *pattern, int flags, int maxMatches ) : mMaxMatches( maxMatches ), mMatchBuffers( 0 ), mMatchLengths( 0 ), mMatchValid( 0 )
{
    const char *errStr;
    int errOffset = 0;
    if ( !(mRegex = pcre_compile( pattern, flags, &errStr, &errOffset, 0 )) )
    {
        Panic( "pcre_compile(%s): %s at %d", pattern, errStr, errOffset );
    }

    mRegextra = pcre_study( mRegex, 0, &errStr );
    if ( errStr )
    {
        Panic( "pcre_study(%s): %s", pattern, errStr );
    }

    if ( (mOk = (bool)mRegex) )
    {
        mMatchVectors = new int[3*mMaxMatches];
        memset( mMatchVectors, 0, sizeof(*mMatchVectors)*3*mMaxMatches );
        mMatchBuffers = new char *[mMaxMatches];
        memset( mMatchBuffers, 0, sizeof(*mMatchBuffers)*mMaxMatches );
        mMatchLengths = new int[mMaxMatches];
        memset( mMatchLengths, 0, sizeof(*mMatchLengths)*mMaxMatches );
        mMatchValid = new bool[mMaxMatches];
        memset( mMatchValid, 0, sizeof(*mMatchValid)*mMaxMatches );
    }
    mNumMatches = 0;
}

RegExpr::~RegExpr()
{
    for ( int i = 0; i < mMaxMatches; i++ )
    {
        if ( mMatchBuffers[i] )
        {
            delete[] mMatchBuffers[i];
        }
    }
    delete[] mMatchValid;
    delete[] mMatchLengths;
    delete[] mMatchBuffers;
    delete[] mMatchVectors;
}

int RegExpr::match( const char *subjectString, int subjectLength, int flags )
{
    mMatchString = subjectString;

    mNumMatches = pcre_exec( mRegex, mRegextra, subjectString, subjectLength, 0, flags, mMatchVectors, 2*mMaxMatches );

    if ( mNumMatches <= 0 )
    {
        if ( mNumMatches < PCRE_ERROR_NOMATCH )
        {
            Error( "Error %d executing regular expression", mNumMatches );
        }
        return( mNumMatches = 0 );
    }

    for( int i = 0; i < mMaxMatches; i++ )
        mMatchValid[i] = false;

    return( mNumMatches );
}

const char *RegExpr::matchString( int matchIndex ) const
{
    if ( matchIndex > mNumMatches )
        return( 0 );

    if ( !mMatchValid[matchIndex] )
    {
        int match_len = mMatchVectors[(2*matchIndex)+1]-mMatchVectors[2*matchIndex];
        if ( mMatchLengths[matchIndex] < (match_len+1) )
        {
            delete[] mMatchBuffers[matchIndex];
            mMatchBuffers[matchIndex] = new char[match_len+1];
            mMatchLengths[matchIndex] = match_len+1;
        }
        memcpy( mMatchBuffers[matchIndex], mMatchString+mMatchVectors[2*matchIndex], match_len );
        mMatchBuffers[matchIndex][match_len] = '\0';
        mMatchValid[matchIndex] = true;
    }
    return( mMatchBuffers[matchIndex] );
}

int RegExpr::matchLength( int matchIndex ) const
{
    if ( matchIndex > mNumMatches )
        return( 0 );

    return( mMatchVectors[(2*matchIndex)+1]-mMatchVectors[2*matchIndex] );
}

#endif // HAVE_LIBPCRE

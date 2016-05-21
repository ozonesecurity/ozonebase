#ifndef LIBGEN_UTILS_H
#define LIBGEN_UTILS_H

#include <string>
#include <vector>
#include <deque>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

inline int max( int a, int b )
{
    return( a>=b?a:b );
}

inline int min( int a, int b )
{
    return( a<=b?a:b );
}

unsigned long getShift( unsigned long divisor, int limit=0 );

#ifndef htobe64
uint64_t htobe64( uint64_t input );
uint64_t be64toh( uint64_t input );
#endif
#ifndef htobe32
uint32_t htole32( uint32_t input );
uint32_t le32toh( uint32_t input );
uint32_t htobe32( uint32_t input );
uint32_t be32toh( uint32_t input );
#endif
uint32_t htobe24( uint32_t input );
uint32_t be24toh( uint32_t input );
#ifndef htobe16
uint16_t htobe16( uint16_t input );
uint16_t be16toh( uint16_t input );
#endif

const std::string stringtf( const char *format, ... ) __attribute__ ((format (printf, 1, 2)));
const std::string stringtf( const std::string &format, ... );

const std::string stringtf( const char *format, ... );
const std::string stringtf( const std::string &format, ... );

std::string toLower( const std::string &upperStr );
std::string toUpper( const std::string &lowerStr );

bool startsWith( const std::string &haystack, const std::string &needle );

typedef std::vector<std::string> StringVector;
StringVector split( const std::string &string, const std::string chars, int limit=0 );

const std::string base64Encode( const std::string &inString );
template <class T> class Buffer;
typedef Buffer<uint8_t> ByteBuffer;
const std::string base64Encode( const ByteBuffer &buffer );
const std::string base64Encode( const unsigned char *data, size_t size );
const std::string base64Decode( const std::string &inString );

bool encodeBCD( const char *digitsIn, ssize_t digitsInLen, unsigned char *digitsOut, ssize_t &digitsOutLen );
bool encodeBCD( const std::string &digitsIn, unsigned char *digitsOut, ssize_t &digitsOutLen );
bool decodeBCD( const unsigned char *digitsIn, ssize_t digitsInLen, char *digitsOut, ssize_t &digitsOutLen );
bool decodeBCD( const unsigned char *digitsIn, ssize_t digitsInLen, std::string &digitsOut );

const char *getStringEnv( const char *varName, const char *defaultValue=0 );
int getIntEnv( const char *varName, const char *defaultValue=0 );
bool getBoolEnv( const char *varName, const char *defaultValue=0 );

class StringTokenList
{
protected:
    typedef char *(*StrFn)( char *, const char * );

public:
    typedef std::deque<std::string> TokenList;

public:
    enum {
        STLF_NONE = 0,      // Split using sep as a whole string to be matched
        STLF_ALTCHARS = 1,  // Split using sep as a collection of chars, any of which can match
        STLF_MULTI = 2      // Ignore multiple consecutive matches with nothing between
    };

private:
    TokenList mTokens;

public:
    StringTokenList( const std::string &str, const std::string &sep, int flags=STLF_NONE )
    {
        splitIntoTokens( str.c_str(), sep.c_str(), flags );
    }
    StringTokenList( const std::string &str, const char *sep, int flags=STLF_NONE )
    {
        splitIntoTokens( str.c_str(), sep, flags );
    }
    StringTokenList( const char *str, const char *sep, int flags=STLF_NONE )
    {
        splitIntoTokens( str, sep, flags );
    }
    StringTokenList( const char *str, const char sep, int flags=STLF_NONE )
    {
        char sepstr[2];
        sepstr[0] = sep;
        sepstr[1] = 0;
        splitIntoTokens( str, sepstr, flags );
    }
    StringTokenList( const std::string &str )
    {
        splitIntoTokens( str.c_str(), " ", STLF_NONE );
    }
    StringTokenList( const char *str )
    {
        splitIntoTokens( str, " ", STLF_NONE );
    }
    StringTokenList( const StringTokenList &tl )
    {
        mTokens = tl.mTokens;
    }
    StringTokenList()
    {
    }

    void operator=( const StringTokenList &tl )
    {
        mTokens = tl.mTokens;
    }
    void operator+=( StringTokenList &tl )
    {
        for( std::deque<std::string>::iterator iter = tl.mTokens.begin(); iter != tl.mTokens.end(); iter++ )
        {
            mTokens.push_back( *iter );
        }
    }
    StringTokenList operator+( StringTokenList &tl ) const
    {
        StringTokenList temp = ( *this );
        temp += tl;
        return temp;
    }
    std::string operator[]( const unsigned int pos )
    {
        if ( pos < mTokens.size() )
        {
            return mTokens[pos];
        }
        else
        {
            return "";
        }
    }
    const TokenList &tokens() const
    {
        return( mTokens );
    }
    TokenList tokens()
    {
        return( mTokens );
    }
    int size() const
    {
        return mTokens.size();
    }

protected:
    void splitIntoTokens( const char* str, const char *sep, int flags=STLF_NONE );

public:
    // This gives us the ability to iterate over the tokens
    typedef std::deque<std::string>::iterator iterator;
    iterator begin()
    {
        return mTokens.begin();
    }
    iterator end()
    {
        return mTokens.end();
    }
};

class SvrException
{
protected:
    typedef enum { INFO, WARNING, ERROR, PANIC } Severity;

protected:
    std::string mError;
    Severity mSeverity;

public:
    SvrException( const std::string &error, Severity severity=ERROR );

public:
    const std::string &getError() const
    {
        return( mError );
    }
    Severity getSeverity() const
    {
        return( mSeverity );
    }
    bool isInfo() const
    {
        return( mSeverity == INFO );
    }
    bool isWarning() const
    {
        return( mSeverity == WARNING );
    }
    bool isError() const
    {
        return( mSeverity == ERROR );
    }
    bool isPanic() const
    {
        return( mSeverity == PANIC );
    }
};

#endif // LIBGEN_UTILS_H

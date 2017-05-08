#define DB_COMP_ID DB_COMP_ID_GENERAL

#include "libgenUtils.h"

#include "libgenDebug.h"
#include "libgenBuffer.h"

#include <stdlib.h>
#include <strings.h>
#include <stdarg.h>
#ifdef __APPLE__
#include <sys/_endian.h>
#else
#include <endian.h>
#endif

// credit: https://gist.github.com/atr000/249599
//
#ifdef __APPLE__
#define __bswap_16(value) \
((((value) & 0xff) << 8) | ((value) >> 8))

#define __bswap_32(value) \
(((uint32_t)__bswap_16((uint16_t)((value) & 0xffff)) << 16) | \
(uint32_t)__bswap_16((uint16_t)((value) >> 16)))

#define __bswap_64(value) \
(((uint64_t)__bswap_32((uint32_t)((value) & 0xffffffff)) \
<< 32) | \
(uint64_t)__bswap_32((uint32_t)((value) >> 32)))
#else
#include <byteswap.h>
#endif



/**
* @brief 
*
* @param divisor
* @param limit
*
* @return 
*/
unsigned long getShift( unsigned long divisor, int limit )
{
    if ( limit && divisor > limit )
    {
        while ( divisor > limit )
            divisor >>= 1;
    }
    //printf( "Frame: %d, adjusting varBlend = %d\n", frameCount, varBlend );
    unsigned long shift = 0;
    for ( unsigned long i = 2; i < 1<<16; i *= 2 )
    {
        shift++;
        if ( divisor <= i )
            break;
    }
    //Fatal( "Illegal divisor %lu, not power of two", divisor );
    return( shift );
}

#ifndef htobe64
uint64_t htobe64( uint64_t input )
{
    return( __bswap_64( input ) );
}

uint64_t be64toh( uint64_t input )
{
    return( __bswap_64( input ) );
}
#endif

#ifndef htobe32
uint32_t htobe32( uint32_t input )
{
    return( __bswap_32( input ) );
}

uint32_t be32toh( uint32_t input )
{
    return( __bswap_32( input ) );
}

uint32_t htole32( uint32_t input )
{
    return( input );
}

uint32_t le32toh( uint32_t input )
{
    return( input );
}
#endif

uint32_t htobe24( uint32_t input )
{
    uint32_t output = htobe32( input ) >> 8;
    return( output );
}

uint32_t be24toh( uint32_t input )
{
    uint32_t output = htobe32( input << 8 );
    return( output );
}

#ifndef htobe16
uint16_t htobe16( uint16_t input )
{
    return( __bswap_16( input ) );
}

uint16_t be16toh( uint16_t input )
{
    return( __bswap_16( input ) );
}
#endif

/**
* @brief 
*
* @param format
* @param ...
*
* @return 
*/
const std::string stringtf( const char *format, ... )
{
    char buffer[BUFSIZ];

    va_list args;
    va_start( args, format );
    int bytesReqd = vsnprintf( buffer, sizeof(buffer), format, args );
    if ( bytesReqd < sizeof(buffer) )
        return( std::string( buffer ) );
    else
    {
        char bigBuffer[bytesReqd+1];
        vsnprintf( bigBuffer, sizeof(bigBuffer), format, args );
        return( std::string( bigBuffer ) );
    }
    va_end( args );
}

/**
* @brief 
*
* @param format
* @param ...
*
* @return 
*/
const std::string stringtf( const std::string &format, ... )
{
    char buffer[BUFSIZ];

    va_list args;
    va_start( args, format );
    //return( stringtf( format.c_str(), args ) );
    int bytesReqd = vsnprintf( buffer, sizeof(buffer), format.c_str(), args );
    if ( bytesReqd < sizeof(buffer) )
        return( std::string( buffer ) );
    else
    {
        char bigBuffer[bytesReqd+1];
        vsnprintf( bigBuffer, sizeof(bigBuffer), format.c_str(), args );
        return( std::string( bigBuffer ) );
    }
    va_end( args );
}

/**
* @brief 
*
* @param upperStr
*
* @return 
*/
std::string toLower( const std::string &upperStr )
{
    std::string lowerStr;
    lowerStr.resize( upperStr.length() );
    for ( int i = 0; i < upperStr.length(); i++ )
    {
        lowerStr[i] = tolower( upperStr[i] );
    }
    return( lowerStr );
}

/**
* @brief 
*
* @param lowerStr
*
* @return 
*/
std::string toUpper( const std::string &lowerStr )
{
    std::string upperStr;
    upperStr.resize( lowerStr.length() );
    for ( int i = 0; i < lowerStr.length(); i++ )
    {
        upperStr[i] = tolower( lowerStr[i] );
    }
    return( upperStr );
}

/**
* @brief 
*
* @param haystack
* @param needle
*
* @return 
*/
bool startsWith( const std::string &haystack, const std::string &needle )
{
    return( haystack.substr( 0, needle.length() ) == needle );
}

/**
* @brief 
*
* @param string
* @param chars
* @param limit
*
* @return 
*/
StringVector split( const std::string &string, const std::string chars, int limit )
{
    StringVector stringVector;
    std::string tempString = string;
    std::string::size_type startIndex = 0;
    std::string::size_type endIndex = 0;

    //Info( "Looking for '%s' in '%s', limit %d", chars.c_str(), string.c_str(), limit );
    do
    {
        // Find delimiters
        endIndex = string.find_first_of( chars, startIndex );
        //Info( "Got endIndex at %d", endIndex );
        if ( endIndex > 0 )
        {
            //Info( "Adding '%s'", string.substr( startIndex, endIndex-startIndex ).c_str() );
            stringVector.push_back( string.substr( startIndex, endIndex-startIndex ) );
        }
        if ( endIndex == std::string::npos )
            break;
        // Find non-delimiters
        startIndex = tempString.find_first_not_of( chars, endIndex );
        if ( limit && (stringVector.size() == (limit-1)) )
        {
            stringVector.push_back( string.substr( startIndex ) );
            break;
        }
        //Info( "Got new startIndex at %d", startIndex );
    } while ( startIndex != std::string::npos );
    //Info( "Finished with %d strings", stringVector.size() );

    return( stringVector );
}

static const std::string base64Chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


/**
* @brief 
*
* @param c
*
* @return 
*/
static inline bool isBase64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

/**
* @brief 
*
* @param 
*
* @return 
*/
const std::string base64Encode( const std::string &inString  )
{
    return( base64Encode( (const unsigned char *)inString.data(), inString.size() ) );
}

/**
* @brief 
*
* @param 
*
* @return 
*/
const std::string base64Encode( const ByteBuffer &buffer  )
{
    return( base64Encode( buffer.data(), buffer.size() ) );
}

/**
* @brief 
*
* @param data
* @param size
*
* @return 
*/
const std::string base64Encode( const unsigned char *data, size_t size )
{
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char charArray3[3];
  unsigned char charArray4[4];

  while (size--) {
    charArray3[i++] = *(data++);
    if (i == 3) {
      charArray4[0] = (charArray3[0] & 0xfc) >> 2;
      charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
      charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
      charArray4[3] = charArray3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64Chars[charArray4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      charArray3[j] = '\0';

    charArray4[0] = (charArray3[0] & 0xfc) >> 2;
    charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
    charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
    charArray4[3] = charArray3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64Chars[charArray4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

/**
* @brief 
*
* @param inString
*
* @return 
*/
const std::string base64Decode( const std::string &inString )
{
    int in_len = inString.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char charArray4[4], charArray3[3];
    std::string ret;

    while (in_len-- && ( inString[in_] != '=') && isBase64(inString[in_]))
    {
        charArray4[i++] = inString[in_]; in_++;
        if ( i == 4 )
        {
            for (i = 0; i <4; i++)
                charArray4[i] = base64Chars.find(charArray4[i]);

            charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

            for (i = 0; (i < 3); i++)
                ret += charArray3[i];
            i = 0;
        }
    }

    if ( i )
    {
        for (j = i; j <4; j++)
            charArray4[j] = 0;

        for (j = 0; j <4; j++)
            charArray4[j] = base64Chars.find(charArray4[j]);

        charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
        charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];

        for (j = 0; (j < i - 1); j++) ret += charArray3[j];
    }

    return( ret );
}

/**
* @brief 
*
* @param digitsIn
* @param digitsInLen
* @param digitsOut
* @param digitsOutLen
*
* @return 
*/
bool encodeBCD( const char *digitsIn, ssize_t digitsInLen, unsigned char *digitsOut, ssize_t &digitsOutLen )
{
    bool good = true;
    digitsOutLen = 0;
    for ( int i = 0; i < digitsInLen; i++ )
    {
        char digit = digitsIn[i] - '0';
        if ( digit >= 0 && digit <= 9 )
            if ( i%2 == 0 )
                digitsOut[digitsOutLen] = digit;
            else
                digitsOut[digitsOutLen++] |= (digit<<4);
        else
            good = false;
    }
    if ( digitsInLen%2 == 1 )
        digitsOut[digitsOutLen++] |= 0xf0;
    return( good );
}

/**
* @brief 
*
* @param digitsIn
* @param digitsOut
* @param digitsOutLen
*
* @return 
*/
bool encodeBCD( const std::string &digitsIn, unsigned char *digitsOut, ssize_t &digitsOutLen )
{
    bool good = true;
    digitsOutLen = 0;
    for ( int i = 0; i < digitsIn.size(); i++ )
    {
        char digit = digitsIn[i] - '0';
        if ( digit >= 0 && digit <= 9 )
            if ( i%2 == 0 )
                digitsOut[digitsOutLen] = digit;
            else
                digitsOut[digitsOutLen++] |= (digit<<4);
        else
            good = false;
    }
    if ( digitsIn.size()%2 == 1 )
        digitsOut[digitsOutLen++] |= 0xf0;
    return( good );
}

/**
* @brief 
*
* @param digitsIn
* @param digitsInLen
* @param digitsOut
* @param digitsOutLen
*
* @return 
*/
bool decodeBCD( const unsigned char *digitsIn, ssize_t digitsInLen, char *digitsOut, ssize_t &digitsOutLen )
{
    bool good = true;
    digitsOutLen = 0;
    for ( int i = 0; i < digitsInLen; i++ )
    {
        unsigned char digit1 = digitsIn[i] & 0x0f;
        if ( digit1 <= 9 )
            digitsOut[digitsOutLen++] = '0'+digit1;
        else
            good = false;
        unsigned char digit2 = digitsIn[i] >> 4;
        if ( digit2 <= 9 )
            digitsOut[digitsOutLen++] = '0'+digit2;
        else if ( digit2 == 0xf )
            break;
        else
            good = false;
    }
    digitsOut[digitsOutLen] = '\0';
    return( good );
}

/**
* @brief 
*
* @param digitsIn
* @param digitsInLen
* @param digitsOut
*
* @return 
*/
bool decodeBCD( const unsigned char *digitsIn, ssize_t digitsInLen, std::string &digitsOut )
{
    bool good = true;
    digitsOut.clear();
    for ( int i = 0; i < digitsInLen; i++ )
    {
        unsigned char digit1 = digitsIn[i] & 0x0f;
        if ( digit1 <= 9 )
            digitsOut += '0'+digit1;
        else
            good = false;
        unsigned char digit2 = digitsIn[i] >> 4;
        if ( digit2 <= 9 )
            digitsOut += '0'+digit2;
        else if ( digit2 == 0xf )
            break;
        else
            good = false;
    }
    return( good );
}

/**
* @brief 
*
* @param varName
* @param defaultValue
*
* @return 
*/
const char *getStringEnv( const char *varName, const char *defaultValue )
{
    const char *envValue = getenv( varName );
    if ( !envValue && !defaultValue ) 
        Panic( "No %s environment variable found", varName );
    return( envValue?envValue:defaultValue );
}

/**
* @brief 
*
* @param varName
* @param defaultValue
*
* @return 
*/
int getIntEnv( const char *varName, const char *defaultValue )
{
    return( strtol( getStringEnv( varName, defaultValue ), NULL, 0 ) );
}

/**
* @brief 
*
* @param varName
* @param defaultValue
*
* @return 
*/
bool getBoolEnv( const char *varName, const char *defaultValue )
{
    const char *envValue = getStringEnv( varName, defaultValue );
    if ( strcasecmp( envValue, "true" ) == 0 )
        return( true );
    if ( strcasecmp( envValue, "yes" ) == 0 )
        return( true );
    if ( strcasecmp( envValue, "y" ) == 0 )
        return( true );
    if ( strcasecmp( envValue, "0" ) == 0 )
        return( false );
    return( (bool)strtol( envValue, NULL, 0 ) );
}

/**
* @brief 
*
* @param str
* @param sep
* @param flags
*/
void StringTokenList::splitIntoTokens( const char* str, const char *sep, int flags )
{
    char *pos;
    char *current = (char*)str;
    //const char *pos;
    //const char *current = (char*)str;

    StrFn strfn = 0;
    int seplen = 0;

    if ( flags & STLF_ALTCHARS )
    {
        strfn = strpbrk;
        seplen = 1;
    }
    else
    {
        strfn = strstr;
        seplen = strlen( sep );
    }

    while( (pos=(*strfn)( current, sep )) )
    {
        int length = pos-current;
        if ( length )
        {
            char temp[length+1];
            memcpy( temp, current, length );
            temp[length]=0;
            mTokens.push_back( std::string( temp ) );
        }
        else if ( !(flags & STLF_MULTI) )
        {
            mTokens.push_back( std::string() );
        }
        current = pos + seplen;
    }

    if ( *current != 0 )
    {
        mTokens.push_back( std::string( current ) );
    }
}

/**
* @brief 
*
* @param error
* @param severity
*/
SvrException::SvrException( const std::string &error, Severity severity ) : mError( error ), mSeverity( severity )
{
    Debug( 6, "New exception '%s' (%d) created", mError.c_str(), mSeverity );
}

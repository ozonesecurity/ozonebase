#ifndef LIBGEN_MEM_UTILS_H
#define LIBGEN_MEM_UTILS_H

inline char *mempbrk( register const char *s, const char *accept, size_t limit )
{
    if ( limit <= 0 || !s || !accept || !*accept )
        return( 0 );

    register int i,j;
    size_t acc_len = strlen( accept );

    for ( i = 0; i < limit; s++, i++ )
    {
        for ( j = 0; j < acc_len; j++ )
        {
            if ( *s == accept[j] )
            {
                return( (char *)s );
            }
        }
    }
    return( 0 );
}

inline char *memstr( register const char *s, const char *n, size_t limit )
{
    if ( limit <= 0 || !s || !n )
        return( 0 );

    if ( !*n )
        return( (char *)s );

    register int i,j,k;
    size_t n_len = strlen( n );

    for ( i = 0; i < limit; i++, s++ )
    {
        if ( *s != *n )
            continue;
        j = 1;
        k = 1;
        while ( true )
        {
            if ( k >= n_len )
                return( (char *)s );
            if ( s[j++] != n[k++] )
                break;
        }
    }
    return( 0 );
}

inline size_t memspn( register const char *s, const char *accept, size_t limit )
{
    if ( limit <= 0 || !s || !accept || !*accept )
        return( 0 );

    register int i,j;
    size_t acc_len = strlen( accept );

    for ( i = 0; i < limit; s++, i++ )
    {
        register bool found = false;
        for ( j = 0; j < acc_len; j++ )
        {
            if ( *s == accept[j] )
            {
                found = true;
                break;
            }
        }
        if ( !found )
        {
            return( i );
        }
    }
    return( limit );
}

inline size_t memcspn( register const char *s, const char *reject, size_t limit )
{
    if ( limit <= 0 || !s || !reject )
        return( 0 );

    if ( !*reject )
        return( limit );

    register int i,j;
    size_t rej_len = strlen( reject );

    for ( i = 0; i < limit; s++, i++ )
    {
        for ( j = 0; j < rej_len; j++ )
        {
            if ( *s == reject[j] )
            {
                return( i );
            }
        }
    }
    return( limit );
}

#endif // LIBGEN_MEM_UTILS_H

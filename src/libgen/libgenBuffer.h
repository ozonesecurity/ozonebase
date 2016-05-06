#ifndef LIBGEN_BUFFER_H
#define LIBGEN_BUFFER_H

#include "libgenDebug.h"

//#include <new>
//#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

template <class T> class Buffer
{
public:
    typedef T ElementType;

protected:
    T *mStorage;
    size_t mAllocation;
    size_t mSize;
    T *mHead;
    T *mTail;
    bool mAdopted;

public:
/*
    static void *operator new( size_t size )
    {
        void *p = ::memalign( size, 16 );
        if ( p == NULL )
            throw std::bad_alloc(); // ANSI/ISO compliant behavior
        return( p );
    }
    static void operator delete( void *p )
    {
        ::free( p );
    }
*/
public:
    Buffer() : mStorage( 0 ), mAllocation( 0 ), mSize( 0 ), mHead( 0 ), mTail( 0 ), mAdopted( false )
    {
    }
    Buffer( size_t size ) : mAllocation( size ), mSize( size ), mAdopted( false )
    {
        mHead = mStorage = new T[mSize];
        memset( mStorage, 0, sizeof(T)*mSize );
        mTail = mHead + mSize;
    }
    Buffer( const T *storage, size_t size ) : mAllocation( size ), mSize( size ), mAdopted( false )
    {
        mHead = mStorage = new T[mSize];
        memcpy( mStorage, storage, sizeof(T)*mSize );
        mTail = mHead + mSize;
    }
    Buffer( const Buffer &buffer ) : mAllocation( buffer.mSize ), mSize( buffer.mSize ), mAdopted( buffer.mAdopted )
    {
        if ( mAdopted )
        {
            mStorage = buffer.mStorage;
            mHead = buffer.mHead;
        }
        else
        {
            mHead = mStorage = new T[mSize];
            memcpy( mStorage, buffer.mHead, sizeof(T)*mSize );
        }
        mTail = mHead + mSize;
    }
    ~Buffer()
    {
        if ( !mAdopted )
            delete[] mStorage;
    }
    inline T *data() const { return( mHead ); }
    inline T *head() const { return( mHead ); }
    inline T *tail() const { return( mTail ); }
    bool adopted() const { return( mAdopted ); }
    inline size_t size() const { return( mSize ); }
    size_t size( size_t size )
    {
        if ( mAdopted )
            Panic( "Cannot alter size for adopted buffer" );
        if ( size > mAllocation )
        {
            reserve( size );
            mSize = mAllocation;
        }
        else if ( size < mAllocation )
        {
            mSize = size;
        }
        mTail = mHead + mSize;
        return( mSize );
    }
    inline size_t length() const { return( mSize ); }
    inline bool empty() const { return( mSize == 0 ); }
    size_t reserve( size_t size )
    {
        if ( mAdopted )
            Panic( "Cannot reserve size for adopted buffer" );
        if ( mSize < size )
            expand( size-mSize );
        return( mSize );
    }
    inline size_t capacity() const { return( mAllocation ); }

    inline void clear()
    {
        mSize = 0;
        mHead = mTail = mStorage;
        mAdopted = false;
    }

    void erase()
    {
        memset( mStorage, 0, mAllocation );
    }

    void fill( T value )
    {
        if ( sizeof(T) == sizeof(char) )
            memset( mStorage, value, sizeof(T)*mSize );
        else
        {
            T *pStor = mHead;
            while( pStor != mTail )
                *pStor++ = value;
        }
    }

    size_t fill( T value, size_t size )
    {
        if ( mAllocation < size )
        {
            if ( mAdopted )
                Panic( "Cannot alter size in fill for adopted buffer" );
            delete[] mStorage;
            mAllocation = size;
            mHead = mStorage = new T[size];
        }
        mSize = size;
        mHead = mStorage;
        mTail = mHead + mSize;
        if ( sizeof(T) == sizeof(char) )
            memset( mHead, value, sizeof(T)*mSize );
        else
        {
            T *pStor = mHead;
            while( pStor != mTail )
                *pStor++ = value;
        }
        return( mSize );
    }

    size_t assign( const T *storage, size_t size )
    {
        if ( mAllocation < size )
        {
            if ( mAdopted )
                Panic( "Cannot reallocate adopted buffer in assign" );
            delete[] mStorage;
            mAllocation = size;
            mStorage = new T[size];
        }
        mSize = size;
        memcpy( mStorage, storage, sizeof(T)*mSize );
        mHead = mStorage;
        mTail = mHead + mSize;
        return( mSize );
    }

    size_t assign( const Buffer &buffer )
    {
        if ( this == &buffer )
            return( mSize );
        return( assign( buffer.mHead, buffer.mSize ) );
    }

    size_t adopt( T *storage, size_t size )
    {
        if ( !mAdopted )
            delete[] mStorage;

        mAdopted = true;
        mAllocation = mSize = size;
        mHead = mStorage = storage;
        mTail = mHead + mSize;
        return( mSize );
    }

    // Trim from the front of the buffer
    size_t consume( size_t count )
    {
        if ( count > mSize )
        {
            Warning( "Attempt to consume %zd bytes of buffer, size is only %zd bytes", count, mSize );
            count = mSize;
        }
        mHead += count;
        mSize -= count;
        tidy( 0 );
        return( count );
    }
    // Trim from the end of the buffer
    size_t shrink( size_t count )
    {
        if ( count > mSize )
        {
            Warning( "Attempt to shrink buffer by %zd bytes, size is only %zd bytes", count, mSize );
            count = mSize;
        }
        mSize -= count;
        if ( mTail > (mHead + mSize) )
            mTail = mHead + mSize;
        tidy( 0 );
        return( count );
    }
    // Add to the end of the buffer
    size_t expand( size_t count )
    {
        if ( mAdopted )
            Panic( "Cannot expand adopted buffer" );
        int spare = mAllocation - mSize;
        int headSpace = mHead - mStorage;
        int tailSpace = spare - headSpace;
        int width = mTail - mHead;
        if ( spare > count )
        {
            if ( tailSpace < count )
            {
                memmove( mStorage, mHead, sizeof(T)*mSize );
                memset( mStorage+sizeof(T)*mSize, 0, sizeof(T)*(mAllocation-mSize) );
                mHead = mStorage;
                mTail = mHead + width;
            }
        }
        else
        {
            mAllocation += count;
            T *newStorage = new T[mAllocation];
            if ( mStorage )
            {
                memcpy( newStorage, mHead, sizeof(T)*mSize );
                memset( newStorage+sizeof(T)*mSize, 0, sizeof(T)*(mAllocation-mSize) );
                delete[] mStorage;
            }
            else
            {
                memset( newStorage, 0, sizeof(T)*mAllocation );
            }
            mStorage = newStorage;
            mHead = mStorage;
            mTail = mHead + width;
        }
        return( mSize );
    }

    // Return pointer to the first size bytes and advance the head
    T *extract( size_t size )
    {
        if ( size > mSize )
        {
            Warning( "Attempt to extract %zd bytes of buffer, size is only %zd bytes", size, mSize );
            size = mSize;
        }
        T *oldHead = mHead;
        mHead += size;
        mSize -= size;
        tidy( 0 );
        return( oldHead );
    }
    // Add bytes to the end of the buffer
    size_t append( const T *storage, size_t size )
    {
        if ( mAdopted )
            Panic( "Cannot append to adopted buffer" );
        expand( size );
        memcpy( mTail, storage, sizeof(T)*size );
        mTail += size;
        mSize += size;
        return( mSize );
    }
    size_t append( T storage )
    {
        return( append( &storage, 1 ) );
    }
    size_t insert( const T *storage, size_t size )
    {
        if ( mAdopted )
            Panic( "Cannot insert into adopted buffer" );
        expand( size );
        memmove( mHead+size, mHead, mSize );
        memcpy( mHead, storage, sizeof(T)*size );
        mTail += size;
        mSize += size;
        return( mSize );
    }
    size_t insert( T storage )
    {
        return( insert( &storage, 1 ) );
    }
#if 0
    size_t append( const char *storage, size_t size )
    {
        return( append( (const T *)storage, size ) );
    }
#endif
    size_t append( const void *storage, size_t size )
    {
        return( append( (const T *)storage, size ) );
    }
    size_t append( const Buffer &buffer )
    {
        return( append( buffer.mHead, buffer.mSize ) );
    }
    void tidy( bool level=0 )
    {
        if ( mHead != mStorage )
        {
            if ( mSize == 0 )
                mHead = mTail = mStorage;
            else if ( level >= 1 )
            {
                if ( mAdopted )
                    return;
                if ( (mHead-mStorage) > mSize )
                {
                    memcpy( mStorage, mHead, sizeof(T)*mSize );
                    mHead = mStorage;
                    mTail = mHead + mSize;
                }
                else if ( level >= 2 )
                {
                    memmove( mStorage, mHead, sizeof(T)*mSize );
                    mHead = mStorage;
                    mTail = mHead + mSize;
                }
            }
        }
    }

    size_t copy( void *dest, size_t size=0 )
    {
        if ( dest )
        {
            if ( size )
            {
                memcpy( dest, mHead, size );
                return( size );
            }
            else
            {
                memcpy( dest, mHead, mSize );
                return( mSize );
            }
        }
        return( 0 );
    }

    // Extract range
    Buffer range( int index, size_t size ) const
    {
        if ( index >= mSize )
            Panic( "Cannot extract range starting at position %d, buffer only %zd long", index, mSize );
        const T *start = mHead + index;
        const T *end = start + size;
        if ( end >= mTail )
            Panic( "Cannot extract range %d + %zd, buffer only %zd long", index, size, mSize );
        return( Buffer( start, size ) );
    }
    Buffer range( const T *start, size_t size ) const
    {
        if ( start < mHead || start >= mTail )
            Panic( "Cannot extract range starting at %p, outside of assigned memory", start );
        const T *end = start + size;
        if ( end >= mTail )
            Panic( "Cannot extract range %zd, buffer only %zd long", size, mSize );
        return( Buffer( start, size ) );
    }

    Buffer &operator=( const Buffer &buffer )
    {
        assign( buffer );
        return( *this );
    }
    Buffer &operator+=( const Buffer &buffer )
    {
        append( buffer );
        return( *this );
    }
    Buffer &operator+=( size_t count )
    {
        //expand( count );
        size( size+count );
        return( *this );
    }
    Buffer &operator-=( size_t count )
    {
        consume( count );
        return( *this );
    }
    T &operator++() // Prefix
    {
        size( mSize+1 );
        return( *(mTail-1) );
    }
    void operator++(int) // Postfix
    {
        size( mSize+1 );
    }
    T operator--() // Prefix
    {
        consume( 1 );
        return( *mHead );
    }
    T operator--(int) // Postfix
    {
        T val = *mHead;
        consume( 1 );
        return( val );
    }
    operator T *() const
    {
        return( mHead );
    }
#if 0
    T *operator &() const
    {
        return( (T *)mHead );
    }
#endif
    operator void *() const
    {
        return( (void *)mHead );
    }
    operator char *() const
    {
        return( (char *)mHead );
    }
    T *operator+(int offset) const
    {
        return( (T *)(mHead+offset) );
    }
    T operator[](int index) const
    {
        return( *(mHead+index) );
    }
    T &operator[](int index)
    {
        return( *(mHead+index) );
    }
    operator int () const
    {
        return( (int)mSize );
    }

    friend Buffer operator+( const Buffer &buffer1, const Buffer &buffer2 )
    {
        Buffer result( buffer1 );
        result += buffer2;
        return( result );
    }
};

typedef Buffer<bool> BoolBuffer;
typedef Buffer<uint8_t> ByteBuffer;
typedef Buffer<char> CharBuffer;
typedef Buffer<int> IntBuffer;
typedef Buffer<uint8_t> Uint8Buffer;
typedef Buffer<uint16_t> Uint16Buffer;
typedef Buffer<uint32_t> Uint32Buffer;

#endif // LIBGEN_BUFFER_H

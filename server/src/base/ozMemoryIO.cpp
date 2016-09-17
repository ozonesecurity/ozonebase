#include "oz.h"
#include "ozMemoryIO.h"

#include "ozFeedFrame.h"
#include "../libgen/libgenTime.h"

#include <sys/stat.h>

#if OZ_MEM_MAPPED
#include <sys/mman.h>
#include <fcntl.h>
#else // OZ_MEM_MAPPED
#include <sys/ipc.h>
#include <sys/shm.h>
#endif // OZ_MEM_MAPPED

/**
* @brief 
*
* @param location
* @param memoryKey
* @param owner
*/
MemoryIO::MemoryIO( const std::string &location, int memoryKey, bool owner ) :
    mLocation( location ),
    mMemoryKey( memoryKey ),
    mOwner( owner ),
#if OZ_MEM_MAPPED
    mMapFd( -1 ),
    //mMemFile( "" ),
#else // OZ_MEM_MAPPED
    mShmId( -1 ),
#endif // OZ_MEM_MAPPED
    mMemSize( 0 ),
    mMemPtr( NULL ),
    mSharedData( NULL ),
    mImageBuffer( NULL )
{
    mMemFile[0] = '\0';
}

/**
* @brief 
*/
MemoryIO::~MemoryIO()
{
    if ( mMemPtr )
        detachMemory();
}

/**
* @brief 
*
* @param sharedData
*
* @return 
*/
bool MemoryIO::queryMemory( SharedData *sharedData )
{
    size_t memSize = sizeof(SharedData);
    unsigned char *memPtr = NULL;

#if OZ_MEM_MAPPED
    char memFile[PATH_MAX] = "";

    //snprintf( memFile, sizeof(memFile), "%s/oz.mmap.%d", config.path_map, mMemoryKey );
    snprintf( memFile, sizeof(memFile), "%s/oz.mmap.%d", mLocation.c_str(), mMemoryKey );
    int mapFd = open( memFile, O_RDWR, (mode_t)0600 );
    if ( mapFd < 0 )
    {
        Debug( 3, "Can't open memory map file %s: %s", memFile, strerror(errno) );
        return( false );
    }
    struct stat mapStat;
    if ( fstat( mapFd, &mapStat ) < 0 )
        Fatal( "Can't stat memory map file %s: %s", memFile, strerror(errno) );
    if ( mapStat.st_size == 0 )
    {
        close( mapFd );
        //unlink( memFile );
        return( false );
    }
    else if ( mapStat.st_size < memSize ) // Is allowed to be larger as not querying whole thing
    {
        Error( "Got unexpected memory map file size %ld, expected %lu", mapStat.st_size, memSize );
    }

    memPtr = (unsigned char *)mmap( NULL, memSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, mapFd, 0 );
    if ( memPtr == MAP_FAILED )
        if ( errno == EAGAIN )
        {
            Debug( 1, "Unable to map file %s (%lu bytes) to locked memory, trying unlocked", memFile, memSize );
            memPtr = (unsigned char *)mmap( NULL, memSize, PROT_READ|PROT_WRITE, MAP_SHARED, mapFd, 0 );
        }
    if ( memPtr == MAP_FAILED )
    {
        Debug( 3, "Can't map file %s (%lu bytes) to memory: %s(%d)", memFile, memSize, strerror(errno), errno );
        if ( munmap( memPtr, memSize ) < 0 )
            Fatal( "Can't munmap: %s", strerror(errno) );
        close( mapFd );
        //unlink( memFile );
        return( false );
    }
#else // OZ_MEM_MAPPED
    int shmId = shmget( (config.shm_key&0xffff0000)|mMemoryKey, memSize, IPC_CREAT|0700 );
    if ( shmId < 0 )
    {
        Debug( 3, "Can't shmget, probably not enough shared memory space free: %s", strerror(errno));
        return( false );
    }
    memPtr = (unsigned char *)shmat( shmId, 0, 0 );
    if ( memPtr < 0 )
    {
        Debug( 3, "Can't shmat: %s", strerror(errno));
        return( false );
    }
#endif // OZ_MEM_MAPPED

    memcpy( sharedData, (SharedData *)memPtr, sizeof(*sharedData) );

    if ( msync( memPtr, memSize, MS_SYNC ) < 0 )
        Error( "Can't msync: %s", strerror(errno) );
    if ( munmap( memPtr, memSize ) < 0 )
        Fatal( "Can't munmap: %s", strerror(errno) );
    close( mapFd );
    //unlink( memFile );

    return( true );
}

/**
* @brief 
*
* @param imageCount
* @param imageFormat
* @param imageWidth
* @param imageHeight
*/
void MemoryIO::attachMemory( int imageCount, AVPixelFormat imageFormat, uint16_t imageWidth, uint16_t imageHeight )
{
    Image tempImage( imageFormat, imageWidth, imageHeight, 0 );
    Debug( 1,"Pixelformat converted from %d to %d", imageFormat, tempImage.pixelFormat() );
    size_t imageSize = tempImage.size();

    mMemSize = sizeof(SharedData)
             + (imageCount*sizeof(uint64_t))
             + (imageCount*imageSize );

    Debug( 1, "mem.size=%d", mMemSize );
#if OZ_MEM_MAPPED
    //snprintf( mMemFile, sizeof(mMemFile), "%s/oz.mmap.%d", config.path_map, mMemoryKey );
    snprintf( mMemFile, sizeof(mMemFile), "%s/oz.mmap.%d", mLocation.c_str(), mMemoryKey );
    mMapFd = open( mMemFile, O_RDWR|O_CREAT, (mode_t)0600 );
    if ( mMapFd < 0 )
        Fatal( "Can't open memory map file %s, probably not enough space free: %s", mMemFile, strerror(errno) );
    struct stat mapStat;
    if ( fstat( mMapFd, &mapStat ) < 0 )
        Fatal( "Can't stat memory map file %s: %s", mMemFile, strerror(errno) );
    if ( mapStat.st_size == 0 )
    {
        // Allocate the size
        if ( ftruncate( mMapFd, mMemSize ) < 0 )
            Fatal( "Can't extend memory map file %s to %d bytes: %s", mMemFile, mMemSize, strerror(errno) );
    }
    else if ( mapStat.st_size != mMemSize )
    {
        Error( "Got unexpected memory map file size %ld, expected %d", mapStat.st_size, mMemSize );
        close( mMapFd );
        if ( mOwner )
            unlink( mMemFile );
    }

    mMemPtr = (unsigned char *)mmap( NULL, mMemSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, mMapFd, 0 );
    if ( mMemPtr == MAP_FAILED )
        if ( errno == EAGAIN )
        {
            Debug( 1, "Unable to map file %s (%d bytes) to locked memory, trying unlocked", mMemFile, mMemSize );
            mMemPtr = (unsigned char *)mmap( NULL, mMemSize, PROT_READ|PROT_WRITE, MAP_SHARED, mMapFd, 0 );
        }
    if ( mMemPtr == MAP_FAILED )
        Fatal( "Can't map file %s (%d bytes) to memory: %s(%d)", mMemFile, mMemSize, strerror(errno), errno );
#else // OZ_MEM_MAPPED
    mShmId = shmget( (config.shm_key&0xffff0000)|mMemoryKey, mMemSize, IPC_CREAT|0700 );
    if ( mShmId < 0 )
    {
        Error( "Can't shmget, probably not enough shared memory space free: %s", strerror(errno));
        exit( -1 );
    }
    mMemPtr = (unsigned char *)shmat( mShmId, 0, 0 );
    if ( mMemPtr < 0 )
    {
        Error( "Can't shmat: %s", strerror(errno));
        exit( -1 );
    }
#endif // OZ_MEM_MAPPED

    mSharedData = (SharedData *)mMemPtr;
    uint64_t *sharedTimestamps = (uint64_t *)((char *)mSharedData + sizeof(SharedData));
    unsigned char *sharedImages = (unsigned char *)((char *)sharedTimestamps + (imageCount*sizeof(uint64_t)));

    if ( mOwner )
    {
        memset( mMemPtr, 0, mMemSize );
        mSharedData->size = sizeof(SharedData);
        mSharedData->valid = true;
        mSharedData->lastWriteIndex = imageCount;
        mSharedData->lastWriteTime = 0;
        mSharedData->imageCount = imageCount;
        mSharedData->imageFormat = tempImage.pixelFormat();
        mSharedData->imageWidth = imageWidth;
        mSharedData->imageHeight = imageHeight;
    }
    else
    {
        if ( !mSharedData->valid )
        {
            Error( "Shared data not initialised by capture daemon" );
            exit( -1 );
        }
    }

    mImageBuffer = new Snapshot[imageCount];
    for ( int i = 0; i < imageCount; i++ )
    {
        mImageBuffer[i].timestamp = &(sharedTimestamps[i]);
        mImageBuffer[i].image = new Image( tempImage.format(), imageWidth, imageHeight, &(sharedImages[i*imageSize]), true );
    }
}

/**
* @brief 
*/
void MemoryIO::detachMemory()
{
    for ( int i = 0; i < mSharedData->imageCount; i++ )
        delete mImageBuffer[i].image;
    delete[] mImageBuffer;

    if ( mOwner )
    {
        mSharedData->valid = false;
        memset( mMemPtr, 0, mMemSize );
    }

#if OZ_MEM_MAPPED
    if ( msync( mMemPtr, mMemSize, MS_SYNC ) < 0 )
        Error( "Can't msync: %s", strerror(errno) );
    if ( munmap( mMemPtr, mMemSize ) < 0 )
        Fatal( "Can't munmap: %s", strerror(errno) );
    close( mMapFd );
    if ( mOwner )
        unlink( mMemFile );
#else // OZ_MEM_MAPPED
    struct shmid_ds shmData;
    if ( shmctl( mShmId, IPC_STAT, &shmData ) < 0 )
    {
        Error( "Can't shmctl: %s", strerror(errno) );
        exit( -1 );
    }
    if ( shmData.shm_nattch <= 1 )
    {
        if ( shmctl( mShmId, IPC_RMID, 0 ) < 0 )
        {
            Error( "Can't shmctl: %s", strerror(errno) );
            exit( -1 );
        }
    }
#endif // OZ_MEM_MAPPED
}

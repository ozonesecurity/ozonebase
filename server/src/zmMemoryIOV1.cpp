#include "zm.h"
#include "zmMemoryIOV1.h"

#include "zmFeedFrame.h"
#include "libgen/libgenTime.h"

#include <sys/stat.h>

#if ZM_MEM_MAPPED
#include <sys/mman.h>
#include <fcntl.h>
#else // ZM_MEM_MAPPED
#include <sys/ipc.h>
#include <sys/shm.h>
#endif // ZM_MEM_MAPPED

MemoryIOV1::MemoryIOV1( const std::string &location, int memoryKey, bool owner ) :
    mLocation( location ),
    mMemoryKey( memoryKey ),
    mOwner( owner ),
#if ZM_MEM_MAPPED
    mMapFd( -1 ),
    //mMemFile( "" ),
#else // ZM_MEM_MAPPED
    mShmId( -1 ),
#endif // ZM_MEM_MAPPED
    mMemSize( 0 ),
    mMemPtr( NULL ),
    mSharedData( NULL ),
    mTriggerData( NULL ),
    mImageBuffer( NULL )
{
    mMemFile[0] = '\0';
}

MemoryIOV1::~MemoryIOV1()
{
    if ( mMemPtr )
        detachMemory();
}

bool MemoryIOV1::queryMemory( SharedData *sharedData )
{
    size_t memSize = sizeof(SharedData);
    unsigned char *memPtr = NULL;

#if ZM_MEM_MAPPED
    char memFile[PATH_MAX] = "";

    //snprintf( memFile, sizeof(memFile), "%s/zm.mmap.%d", config.path_map, mMemoryKey );
    snprintf( memFile, sizeof(memFile), "%s/zm.mmap.%d", mLocation.c_str(), mMemoryKey );
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
        Error( "Got unexpected memory map file size %lu, expected %lu", mapStat.st_size, memSize );
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
#else // ZM_MEM_MAPPED
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
#endif // ZM_MEM_MAPPED

    memcpy( sharedData, (SharedData *)memPtr, sizeof(*sharedData) );

    if ( msync( memPtr, memSize, MS_SYNC ) < 0 )
        Error( "Can't msync: %s", strerror(errno) );
    if ( munmap( memPtr, memSize ) < 0 )
        Fatal( "Can't munmap: %s", strerror(errno) );
    close( mapFd );
    //unlink( memFile );

    return( true );
}

void MemoryIOV1::attachMemory( int imageCount, PixelFormat imageFormat, uint16_t imageWidth, uint16_t imageHeight )
{
    if ( mMemPtr )
        Fatal( "Unable to attach to shared memory, already attached" );
    mImageCount = imageCount;
    Image tempImage( imageFormat, imageWidth, imageHeight, 0 );
    Info( "Pixelformat converted from %d to %d", imageFormat, tempImage.pixelFormat() );
    size_t imageSize = tempImage.size();

    mMemSize = sizeof(SharedData)
             + sizeof(TriggerData)
             + (mImageCount*sizeof(struct timeval))
             + (mImageCount*imageSize);

    Debug( 1, "mem.size=%d", mMemSize );
#if ZM_MEM_MAPPED
    //snprintf( mMemFile, sizeof(mMemFile), "%s/zm.mmap.%d", config.path_map, mMemoryKey );
    snprintf( mMemFile, sizeof(mMemFile), "%s/zm.mmap.%d", mLocation.c_str(), mMemoryKey );
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
#else // ZM_MEM_MAPPED
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
#endif // ZM_MEM_MAPPED

    mSharedData = (SharedData *)mMemPtr;
    mTriggerData = (TriggerData *)((char *)mSharedData + sizeof(SharedData));
    struct timeval *sharedTimestamps = (struct timeval *)((char *)mTriggerData + sizeof(TriggerData));
    unsigned char *sharedImages = (unsigned char *)((char *)sharedTimestamps + (mImageCount*sizeof(struct timeval)));


    if ( mOwner )
    {
        memset( mMemPtr, 0, mMemSize );
        mSharedData->size = sizeof(SharedData);
        mSharedData->valid = true;
        mSharedData->active = true;
        mSharedData->signal = false;
        mSharedData->state = IDLE;
        mSharedData->last_write_index = mImageCount;
        mSharedData->last_read_index = mImageCount;
        mSharedData->last_write_time = 0;
        mSharedData->last_event = 0;
        mSharedData->action = 0;
        mSharedData->brightness = -1;
        mSharedData->hue = -1;
        mSharedData->colour = -1;
        mSharedData->contrast = -1;
        mSharedData->alarm_x = -1;
        mSharedData->alarm_y = -1;
        mTriggerData->size = sizeof(TriggerData);
        mTriggerData->trigger_state = TRIGGER_CANCEL;
        mTriggerData->trigger_score = 0;
        mTriggerData->trigger_cause[0] = 0;
        mTriggerData->trigger_text[0] = 0;
        mTriggerData->trigger_showtext[0] = 0;
    }
    else
    {
        if ( !mSharedData->valid )
        {
            Error( "Shared data not initialised by capture daemon" );
            exit( -1 );
        }
    }

    mImageBuffer = new Snapshot[mImageCount];
    for ( int i = 0; i < imageCount; i++ )
    {
        mImageBuffer[i].timestamp = &(sharedTimestamps[i]);
        mImageBuffer[i].image = new Image( tempImage.format(), imageWidth, imageHeight, &(sharedImages[i*imageSize]), true );
    }
}

void MemoryIOV1::detachMemory()
{
    if ( !mMemPtr )
        Fatal( "Unable to detach memory, not attached" );

    for ( int i = 0; i < mImageCount; i++ )
        delete mImageBuffer[i].image;
    delete[] mImageBuffer;

    if ( mOwner )
    {
        mSharedData->valid = false;
        memset( mMemPtr, 0, mMemSize );
    }

#if ZM_MEM_MAPPED
    if ( msync( mMemPtr, mMemSize, MS_SYNC ) < 0 )
        Error( "Can't msync: %s", strerror(errno) );
    if ( munmap( mMemPtr, mMemSize ) < 0 )
        Fatal( "Can't munmap: %s", strerror(errno) );
    close( mMapFd );
    if ( mOwner )
        unlink( mMemFile );
#else // ZM_MEM_MAPPED
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
#endif // ZM_MEM_MAPPED
    mMemPtr = NULL;
}

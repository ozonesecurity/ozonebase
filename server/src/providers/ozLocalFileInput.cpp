#include "../base/oz.h"
#include "ozLocalFileInput.h"

#include "../base/ozFfmpeg.h"
#include "../libgen/libgenDebug.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <glob.h>

/**
* @brief 
*
* @param name
* @param pattern
* @param frameRate
*/
LocalFileInput::LocalFileInput( const std::string &name, const std::string &pattern, const FrameRate &frameRate ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mPattern( pattern ),
    mWidth( 0 ),
    mHeight( 0 ),
    mFrameRate( frameRate ),
    mPixelFormat( PIX_FMT_NONE )
{
}

/**
* @brief 
*/
LocalFileInput::~LocalFileInput()
{
}

/**
* @brief 
*
* @return 
*/
int LocalFileInput::run()
{
    glob_t pglob;
    int globStatus = glob( mPattern.c_str(), 0, 0, &pglob );
    if ( globStatus != 0 )
    {
        if ( globStatus < 0 )
        {
            Fatal( "Can't glob '%s': %s", mPattern.c_str(), strerror(errno) );
        }
        else
        {
            Error( "Can't glob '%s': %d", mPattern.c_str(), globStatus );
            setError();
        }
    }
    else
    {
        uint64_t timeInterval = mFrameRate.intervalUsec();
        struct timeval now;
        gettimeofday( &now, NULL );
        uint64_t currTime = (1000000LL*now.tv_sec)+now.tv_usec;
        uint64_t nextTime = currTime;
		
        for ( int i = 0; i < pglob.gl_pathc && !mStop; i++ )
        {
            // Synchronise the output with the desired output frame rate
            while ( currTime < nextTime )
            {
                gettimeofday( &now, 0 );
                currTime = (1000000LL*now.tv_sec)+now.tv_usec;
                usleep( INTERFRAME_TIMEOUT );
            }
            nextTime += timeInterval;

            char *diagPath = pglob.gl_pathv[i];
            Image image( diagPath );
            if ( !mWidth || !mHeight )
            {
                mWidth = image.width();
                mHeight = image.height();
                mPixelFormat = image.pixelFormat();
            }

            VideoFrame *videoFrame = new VideoFrame( this, ++mFrameCount, currTime, image.buffer() );
            distributeFrame( FramePtr( videoFrame ) );
        }
	}

    globfree( &pglob );
    cleanup();
    return( !ended() );
}

#define DB_COMP_ID DB_COMP_ID_THREADS

#include "libgenThread.h"

#include "libgenDebug.h"

#include <errno.h>
#include <sys/time.h>

struct timespec getTimeout( int secs )
{
    struct timespec timeout;
    struct timeval temp_timeout;
    gettimeofday( &temp_timeout, 0 );
    timeout.tv_sec = temp_timeout.tv_sec + secs;
    timeout.tv_nsec = temp_timeout.tv_usec*1000;
    return( timeout );
}

struct timespec getTimeout( double secs )
{
    struct timespec timeout;
    struct timeval temp_timeout;
    gettimeofday( &temp_timeout, 0 );
    timeout.tv_sec = temp_timeout.tv_sec + int(secs);
    timeout.tv_nsec = temp_timeout.tv_usec += (long int)(1000000000.0*(secs-int(secs)));
    if ( timeout.tv_nsec > 1000000000 )
    {
        timeout.tv_sec += 1;
        timeout.tv_nsec -= 1000000000;
    }
    return( timeout );
}

Mutex::Mutex()
{
    if ( pthread_mutex_init( &mMutex, NULL ) < 0 )
        throw ThreadException( stringtf( "Unable to create pthread mutex: %s", strerror(errno) ) );
}

Mutex::~Mutex()
{
    if ( locked() )
        Warning( "Destroying mutex when locked" );
    if ( pthread_mutex_destroy( &mMutex ) < 0 )
        throw ThreadException( stringtf( "Unable to destroy pthread mutex: %s", strerror(errno) ) );
}

void Mutex::lock()
{
    if ( pthread_mutex_lock( &mMutex ) < 0 )
        throw ThreadException( stringtf( "Unable to lock pthread mutex: %s", strerror(errno) ) );
}

void Mutex::lock( int secs )
{
    struct timespec timeout = getTimeout( secs );
    if ( pthread_mutex_timedlock( &mMutex, &timeout ) < 0 )
        throw ThreadException( stringtf( "Unable to timedlock pthread mutex: %s", strerror(errno) ) );
}

void Mutex::lock( double secs )
{
    struct timespec timeout = getTimeout( secs );
    if ( pthread_mutex_timedlock( &mMutex, &timeout ) < 0 )
        throw ThreadException( stringtf( "Unable to timedlock pthread mutex: %s", strerror(errno) ) );
}

void Mutex::unlock()
{
    if ( pthread_mutex_unlock( &mMutex ) < 0 )
        throw ThreadException( stringtf( "Unable to unlock pthread mutex: %s", strerror(errno) ) );
}

bool Mutex::locked()
{
    int state = pthread_mutex_trylock( &mMutex );
    if ( state != 0 && state != EBUSY )
        throw ThreadException( stringtf( "Unable to trylock pthread mutex: %s", strerror(errno) ) );
    if ( state != EBUSY )
        unlock();
    return( state == EBUSY );
}

ScopedMutex::ScopedMutex( Mutex &mutex ) : mMutex( mutex )
{
    mMutex.lock();
}

ScopedMutex::~ScopedMutex()
{
    mMutex.unlock();
}

Condition::Condition( Mutex &mutex ) : mMutex( mutex )
{
    if ( pthread_cond_init( &mCondition, NULL ) < 0 )
        throw ThreadException( stringtf( "Unable to create pthread condition: %s", strerror(errno) ) );
}

Condition::~Condition()
{
    if ( pthread_cond_destroy( &mCondition ) < 0 )
        throw ThreadException( stringtf( "Unable to destroy pthread condition: %s", strerror(errno) ) );
}

void Condition::wait()
{
    // Locking done outside of this function
    if ( pthread_cond_wait( &mCondition, mMutex.getMutex() ) < 0 )
        throw ThreadException( stringtf( "Unable to wait pthread condition: %s", strerror(errno) ) );
}

bool Condition::wait( int secs )
{
    // Locking done outside of this function
    Debug( 3, "Waiting for %d seconds", secs );
    struct timespec timeout = getTimeout( secs );
    if ( pthread_cond_timedwait( &mCondition, mMutex.getMutex(), &timeout ) < 0 && errno != ETIMEDOUT )
        throw ThreadException( stringtf( "Unable to timedwait pthread condition: %s", strerror(errno) ) );
    return( errno != ETIMEDOUT );
}

bool Condition::wait( double secs )
{
    // Locking done outside of this function
    struct timespec timeout = getTimeout( secs );
    if ( pthread_cond_timedwait( &mCondition, mMutex.getMutex(), &timeout ) < 0 && errno != ETIMEDOUT )
        throw ThreadException( stringtf( "Unable to timedwait pthread condition: %s", strerror(errno) ) );
    return( errno != ETIMEDOUT );
}

void Condition::signal()
{
    if ( pthread_cond_signal( &mCondition ) < 0 )
        throw ThreadException( stringtf( "Unable to signal pthread condition: %s", strerror(errno) ) );
}

void Condition::broadcast()
{
    if ( pthread_cond_broadcast( &mCondition ) < 0 )
        throw ThreadException( stringtf( "Unable to broadcast pthread condition: %s", strerror(errno) ) );
}

Thread::Thread( const std::string &threadLabel ) :
    mThreadLabel( threadLabel ),
    mThreadCondition( mThreadMutex ),
    mTid( -1 ),
    mRunning( false ),
    mStop( false )
{
    Debug( 1, "Creating thread %s", mThreadLabel.c_str() );
}

Thread::Thread( const char *threadLabel ) :
    mThreadLabel( threadLabel ),
    mThreadCondition( mThreadMutex ),
    mTid( -1 ),
    mRunning( false ),
    mStop( false )
{
    Debug( 1, "Creating thread %s", mThreadLabel.c_str() );
}

Thread::~Thread()
{
    Debug( 1, "Destroying thread %d (%s)", mTid, mThreadLabel.c_str() );
    if ( mRunning )
    {
        Warning( "Destroying still running thread %d (%s)", mTid, mThreadLabel.c_str() );
        //join();
    }
}

void *Thread::mThreadFunc( void *arg )
{
    Debug( 2, "Invoking thread" );

    void *status = 0;
    try
    {
        Thread *thisPtr = (Thread *)arg;
        thisPtr->mThreadMutex.lock();
        thisPtr->mTid = thisPtr->id();
        thisPtr->mRunning = true;
        thisPtr->mThreadCondition.signal();
        thisPtr->mThreadMutex.unlock();
        status = (void *)(thisPtr->run());
        Debug( 2, "Exiting thread (%s), status %p", thisPtr->mThreadLabel.c_str(), status );
    }
    catch ( const ThreadException &e )
    {
        Error( "%s", e.getMessage().c_str() );
        status = (void *)-1;
        Debug( 2, "Exiting thread after exception, status %p", status );
    }
    return( status );
}

void Thread::start()
{
    Debug( 1, "Starting thread (%s)", mThreadLabel.c_str() );
    if ( isThread() )
        throw ThreadException( "Can't self start thread" );
    mThreadMutex.lock();
    if ( !mRunning )
    {
        pthread_attr_t threadAttrs;
        pthread_attr_init( &threadAttrs );
        pthread_attr_setscope( &threadAttrs, PTHREAD_SCOPE_SYSTEM );

        if ( pthread_create( &mThread, &threadAttrs, mThreadFunc, this ) < 0 )
            throw ThreadException( stringtf( "Can't create thread: %s", strerror(errno) ) );
        pthread_attr_destroy( &threadAttrs );
        while ( !mRunning )
            mThreadCondition.wait();
        Debug( 1, "Started thread %d", mTid );
    }
    else
    {
        Error( "Attempt to start already running thread %d (%s)", mTid, mThreadLabel.c_str() );
    }
    mThreadMutex.unlock();
}


void Thread::stop()
{
    Debug( 1, "Stopping thread %d (%s)", mTid, mThreadLabel.c_str() );
    mStop = true;
}

void Thread::join()
{
    Debug( 1, "Joining thread %d (%s)", mTid, mThreadLabel.c_str() );
    if ( isThread() )
        throw ThreadException( "Can't self join thread" );
    mThreadMutex.lock();
    if ( mTid >= 0 )
    {
        if ( mRunning )
        {
            void *threadStatus = 0;
            if ( pthread_join( mThread, &threadStatus ) < 0 )
                throw ThreadException( stringtf( "Can't join sender thread: %s", strerror(errno) ) );
            mRunning = false;
            Debug( 1, "Thread %d (%s) exited, status %p", mTid, mThreadLabel.c_str(), threadStatus );
        }
        else
        {
            Warning( "Attempt to join already finished thread %d (%s)", mTid, mThreadLabel.c_str() );
        }
    }
    else
    {
        Warning( "Attempt to join non-started thread %d (%s)", mTid, mThreadLabel.c_str() );
    }
    mThreadMutex.unlock();
    Debug( 1, "Joined thread %d (%s)", mTid, mThreadLabel.c_str() );
}

// Some explicit template instantiations
#include "libgenThreadData.cpp"

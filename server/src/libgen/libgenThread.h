#ifndef LIBGEN_THREAD_H
#define LIBGEN_THREAD_H

#include "libgenException.h"
#include "libgenUtils.h"

#include <string>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

// missing phread_mutex_timedlock
//https://lists.freedesktop.org/archives/spice-devel/2010-September/001231.html
#ifdef __APPLE__
#include <pthread.h>
#include <errno.h>
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec
*abs_timeout)
{
    int pthread_rc;
    struct timespec remaining, slept, ts;

    remaining = *abs_timeout;
    while ((pthread_rc = pthread_mutex_trylock(mutex)) == EBUSY) {
        ts.tv_sec = 0;
        ts.tv_nsec = (remaining.tv_sec > 0 ? 10000000 : min(remaining.tv_nsec,10000000));
        nanosleep(&ts, &slept);
        ts.tv_nsec -= slept.tv_nsec;
        if (ts.tv_nsec <= remaining.tv_nsec) {
            remaining.tv_nsec -= ts.tv_nsec;
        }
        else {
            remaining.tv_sec--;
            remaining.tv_nsec = (1000000 - (ts.tv_nsec - remaining.tv_nsec));
        }
        if (remaining.tv_sec < 0 || (!remaining.tv_sec && remaining.tv_nsec <= 0)) {
            return ETIMEDOUT;
        }
    }

    return pthread_rc;
}
#endif

class ThreadException : public Exception
{
public:
    ThreadException( const std::string &message ) : Exception( stringtf( "(%d) "+message, (pid_t)syscall(SYS_gettid) ) )
    {
    }
};

/// Class representing phread mutex
class Mutex
{
friend class Condition;

private:
    pthread_mutex_t mMutex;

public:
    Mutex();
    ~Mutex();

private:
    Mutex( const Mutex & );             ///< Cannot be copied
    Mutex operator=( const Mutex & );   ///< Cannot be assigned
    pthread_mutex_t *getMutex()
    {
        return( &mMutex );
    }

public:
    void lock();
    void lock( int secs );
    void lock( double secs );
    void unlock();
    bool locked();
};

/// Class wrapping Mutex in auto delete accrding to scope
class ScopedMutex
{
private:
    Mutex &mMutex;

public:
    ScopedMutex( Mutex &mutex );
    ~ScopedMutex();

private:
    ScopedMutex( const ScopedMutex & );             ///< Cannot be copied
    ScopedMutex operator=( const ScopedMutex & );   ///< Cannot be assigned
};

typedef ScopedMutex AutoMutex;

class Condition
{
private:
    Mutex &mMutex;
    pthread_cond_t mCondition;

public:
    Condition( Mutex &mutex );
    ~Condition();

    void wait();
    bool wait( int secs );
    bool wait( double secs );
    void signal();
    void broadcast();
};

class Semaphore : public Condition
{
private:
    Mutex mMutex;

public:
    Semaphore() : Condition( mMutex )
    {
    }

    void wait()
    {
        mMutex.lock();
        Condition::wait();
        mMutex.unlock();
    }
    bool wait( int secs )
    {
        mMutex.lock();
        bool result = Condition::wait( secs );
        mMutex.unlock();
        return( result );
    }
    bool wait( double secs )
    {
        mMutex.lock();
        bool result = Condition::wait( secs );
        mMutex.unlock();
        return( result );
    }
    void signal()
    {
        mMutex.lock();
        Condition::signal();
        mMutex.unlock();
    }
    void broadcast()
    {
        mMutex.lock();
        Condition::broadcast();
        mMutex.unlock();
    }
};

template <class T> class ThreadData
{
private:
    T mValue;
    mutable bool mChanged;
    mutable Mutex mMutex;
    mutable Condition mCondition;

public:
    ThreadData() : mCondition( mMutex )
    {
    }
    ThreadData( T value ) : mValue( value ), mCondition( mMutex )
    {
    }
    //~ThreadData() {}

    operator T() const
    {
        return( getValue() );
    }
    const T operator=( const T value )
    {
        return( setValue( value ) );
    }

    const T getValueImmediate() const
    {
        return( mValue );
    }
    T setValueImmediate( const T value )
    {
        return( mValue = value );
    }
    const T getValue() const;
    T setValue( const T value );
    const T getUpdatedValue() const;
    const T getUpdatedValue( double secs ) const;
    const T getUpdatedValue( int secs ) const;
    void updateValueSignal( const T value );
    void updateValueBroadcast( const T value );
};

template <class T> const T ThreadData<T>::getValue() const
{
    mMutex.lock();
    const T valueCopy = mValue;
    mMutex.unlock();
    return( valueCopy );
}

template <class T> T ThreadData<T>::setValue( const T value )
{
    mMutex.lock();
    const T valueCopy = mValue = value;
    mMutex.unlock();
    return( valueCopy );
}

template <class T> const T ThreadData<T>::getUpdatedValue() const
{
    //Debug( 3, "Waiting for value update, %p", this );
    mMutex.lock();
    mChanged = false;
    //do {
        mCondition.wait();
    //} while ( !mChanged );
    const T valueCopy = mValue;
    mMutex.unlock();
    //Debug( 4, "Got value update, %p", this );
    return( valueCopy );
}

template <class T> const T ThreadData<T>::getUpdatedValue( double secs ) const
{
    //Debug( 3, "Waiting for value update, %.2f secs, %p", secs, this );
    mMutex.lock();
    mChanged = false;
    //do {
        mCondition.wait( secs );
    //} while ( !mChanged );
    const T valueCopy = mValue;
    mMutex.unlock();
    //Debug( 4, "Got value update, %p", this );
    return( valueCopy );
}

template <class T> const T ThreadData<T>::getUpdatedValue( int secs ) const
{
    //Debug( 3, "Waiting for value update, %d secs, %p", secs, this );
    mMutex.lock();
    mChanged = false;
    //do {
        mCondition.wait( secs );
    //} while ( !mChanged );
    const T valueCopy = mValue;
    mMutex.unlock();
    //Debug( 4, "Got value update, %p", this );
    return( valueCopy );
}

template <class T> void ThreadData<T>::updateValueSignal( const T value )
{
    //Debug( 3, "Updating value with signal, %p", this );
    mMutex.lock();
    mValue = value;
    mChanged = true;
    mCondition.signal();
    mMutex.unlock();
    //Debug( 4, "Updated value, %p", this );
}

template <class T> void ThreadData<T>::updateValueBroadcast( const T value )
{
    //Debug( 3, "Updating value with broadcast, %p", this );
    mMutex.lock();
    mValue = value;
    mChanged = true;
    mCondition.broadcast();
    mMutex.unlock();
    //Debug( 4, "Updated value, %p", this );
}

/// Abstract class representing pthread thread.
class Thread
{
public:
    typedef void *(*ThreadFunc)( void * );

protected:
    pthread_t mThread;          ///< Base pthread data

    std::string mThreadLabel;   ///< Label to help identify the thread
    Mutex mThreadMutex;         ///< Mutex protecting this thread from concurrent access
    Condition mThreadCondition; ///< Used to signal thread states to invoking process/thread
    pid_t mTid;                 ///< Thread id
    bool  mRunning;             ///< Flag indicating whether this thread has begun and is running
    bool  mStop;                ///< Flag indicating whether this thread has been signalled to stop

protected:
    Thread( const std::string &threadLabel );
    Thread( const char *threadLabel="..." );
    virtual ~Thread();

    pid_t id() const
    {
        return( (pid_t)syscall(SYS_gettid) );
    }
    void exit( long int status = 0 )
    {
        pthread_exit( (void *)status );
    }
    static void *mThreadFunc( void *arg );

public:
    virtual int run() = 0;      ///< Thread code, must be overriden to do actual thread work

    pid_t tid() const           ///< Return the thread id
    {
        return( mTid );
    }
    void start();               ///< Start the thread and invoke the run method, called by invoker
    void stop();                ///< Stop the thread, by setting the mStop flag. Thread is responsible for actually existing.
    void join();                ///< Wait for the thread to terminate.
    bool isThread()
    {
        return( mTid > -1 && pthread_equal( pthread_self(), mThread ) );
    }
    bool running() const        ///< Indicate whether the thread is running or not
    {
        return( mRunning );
    }
    bool stopped() const        ///< Indicate whether the thread has been signalled to stop
    {
        return( mStop );
    }
};

#endif // LIBGEN_THREAD_H

#include "libgenTimer.h"

#include "libgenDebug.h"

int Timer::TimerThread::mNextTimerId = 0;

/**
* @brief 
*
* @param timer
* @param duration
* @param repeat
*/
Timer::TimerThread::TimerThread( Timer &timer, int duration, bool repeat ) :
    mTimerId( 0 ),
    mTimer( timer ),
    mDuration( duration ),
    mRepeat( repeat ),
    mReset( false ),
    mExpiryFlag( true )
{
    mAccessMutex.lock();
    mTimerId = mNextTimerId++;
    Debug( 5, "Creating timer %d for %d seconds%s", mTimerId, mDuration, mRepeat?", repeating":"" );
    mAccessMutex.unlock();
}

/**
* @brief 
*/
Timer::TimerThread::~TimerThread()
{
    cancel();
}

/**
* @brief 
*/
void Timer::TimerThread::cancel()
{
    mAccessMutex.lock();
    if ( mRunning )
    {
        Debug( 4, "Cancelling timer %d", mTimerId );
        mRepeat = false;
        mReset = false;
        mExpiryFlag.updateValueSignal( false );
    }
    mAccessMutex.unlock();
}

/**
* @brief 
*/
void Timer::TimerThread::reset()
{
    mAccessMutex.lock();
    if ( mRunning )
    {
        Debug( 4, "Resetting timer" );
        mReset = true;
        mExpiryFlag.updateValueSignal( false );
    }
    else
    {
        Error( "Attempting to reset expired timer %d", mTimerId );
    }
    mAccessMutex.unlock();
}

/**
* @brief 
*
* @return 
*/
int Timer::TimerThread::run()
{
    Debug( 4, "Starting timer %d for %d seconds", mTimerId, mDuration );
    bool timerExpired = false;
    do
    {
        mAccessMutex.lock();
        mReset = false;
        mExpiryFlag.setValue( true );
        mAccessMutex.unlock();
        timerExpired = mExpiryFlag.getUpdatedValue( mDuration );
        mAccessMutex.lock();
        if ( timerExpired )
        {
            Debug( 4, "Timer %d expired", mTimerId );
            mTimer.expire();
        }
        else
        {
            Debug( 4, "Timer %d %s", mTimerId, mReset?"reset":"cancelled" );
        }
        mAccessMutex.unlock();
    } while ( mRepeat || (mReset && !timerExpired) );
    return( timerExpired );
}

/**
* @brief 
*
* @param timeout
* @param repeat
*/
Timer::Timer( int timeout, bool repeat ) : mTimerThread( *this, timeout, repeat )
{
    mTimerThread.start();
}

/**
* @brief 
*/
Timer::~Timer()
{
    //cancel();
}

/**
* @brief 
*/
void Timer::Timer::cancel()
{
    mTimerThread.cancel();
}

/**
* @brief 
*/
void Timer::Timer::reset()
{
    mTimerThread.reset();
}


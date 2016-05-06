#ifndef LIBGEN_TIMER_H
#define LIBGEN_TIMER_H

#include "libgenThread.h"

class Timer
{
private:
    class TimerException : public Exception
    {
    public:
        TimerException( const std::string &message ) : Exception( stringtf( "(%d) "+message, (long int)syscall(224) ) )
        {
        }
    };

    class TimerThread : public Thread
    {
    private:
        typedef ThreadData<bool> ExpiryFlag;

    private:
        static int mNextTimerId;

    private:
        int mTimerId;
        Timer &mTimer;
        int mDuration;
        int mRepeat;
        int mReset;
        ExpiryFlag mExpiryFlag;
        Mutex mAccessMutex;

    private:
        void quit()
        {
            cancel();
        }

    public:
        TimerThread( Timer &timer, int timeout, bool repeat );
        ~TimerThread();

        void cancel();
        void reset();
        int run();
    };

protected:
    TimerThread mTimerThread;

protected:
    Timer( int timeout, bool repeat=false );

public:
    virtual ~Timer();

protected:
    virtual void expire()=0;

public:
    void cancel();
    void reset();
};

#endif // LIBGEN_TIMER_H

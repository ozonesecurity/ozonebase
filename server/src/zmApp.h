#ifndef ZM_APP_H
#define ZM_APP_H

#include "libgen/libgenThread.h"
#include <deque>

class Application
{
protected:
    typedef std::deque<Thread *> ThreadList;

protected:
    ThreadList      mThreads;

public:
    Application();
    ~Application() {}

    void addThread( Thread * );
    void run();
};

#endif // ZM_APP_H

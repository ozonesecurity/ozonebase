/** @addtogroup Application */
/*@{*/
#ifndef OZ_APP_H
#define OZ_APP_H

#include <deque>
#include "../libgen/libgenThread.h"

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

#endif // OZ_APP_H
/*@}*/

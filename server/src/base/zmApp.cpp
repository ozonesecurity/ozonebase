#include "zm.h"
#include "zmApp.h"

#include "zmSignal.h"

#include <stdlib.h>

Application::Application()
{
    srand( time( NULL ) );
    signal(SIGPIPE, SIG_IGN);

    zmSetDefaultTermHandler();
    //setDefaultDieHandler();
}

void Application::addThread( Thread *thread )
{
    mThreads.push_back( thread );
}

void Application::run()
{
    for ( ThreadList::iterator iter = mThreads.begin(); iter != mThreads.end(); iter++ )
        (*iter)->start();

    while( !gZmTerminate )
        sleep( 1 );

    for ( ThreadList::iterator iter = mThreads.begin(); iter != mThreads.end(); iter++ )
        (*iter)->stop();

    for ( ThreadList::iterator iter = mThreads.begin(); iter != mThreads.end(); iter++ )
        (*iter)->join();
}

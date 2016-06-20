#include "oz.h"
#include "ozApp.h"

#include "ozSignal.h"

#include <stdlib.h>

/**
* @brief 
*/
Application::Application()
{
    srand( time( NULL ) );
    signal(SIGPIPE, SIG_IGN);

    ozSetDefaultTermHandler();
    //setDefaultDieHandler();
}

/**
* @brief 
*
* @param thread
*/
void Application::addThread( Thread *thread )
{
    mThreads.push_back( thread );
}

/**
* @brief 
*/
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

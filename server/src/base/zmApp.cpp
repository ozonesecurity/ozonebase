#include "zm.h"
#include "zmApp.h"

#include "zmSignal.h"

#include <stdlib.h>

/**
   Write description of function here.
   The function should follow these comments.
   Use of "brief" tag is optional. (no point to it)
  
   The function arguments listed with "param" will be compared 
   to the declaration and verified.

   @param[in]     _inArg1 Description of first function argument.
   @param[out]    _outArg2 Description of second function argument.
   @param[in,out] _inoutArg3 Description of third function argument.
   @return Description of returned value.
 */
Application::Application()
{
    srand( time( NULL ) );
    signal(SIGPIPE, SIG_IGN);

    zmSetDefaultTermHandler();
    //setDefaultDieHandler();
}

/**
   Write description of function here.
   The function should follow these comments.
   Use of "brief" tag is optional. (no point to it)
  
   The function arguments listed with "param" will be compared 
   to the declaration and verified.

   @param[in]     _inArg1 Description of first function argument.
   @param[out]    _outArg2 Description of second function argument.
   @param[in,out] _inoutArg3 Description of third function argument.
   @return Description of returned value.
 */
void Application::addThread( Thread *thread )
{
    mThreads.push_back( thread );
}

/**
   Write description of function here.
   The function should follow these comments.
   Use of "brief" tag is optional. (no point to it)
  
   The function arguments listed with "param" will be compared 
   to the declaration and verified.

   @param[in]     _inArg1 Description of first function argument.
   @param[out]    _outArg2 Description of second function argument.
   @param[in,out] _inoutArg3 Description of third function argument.
   @return Description of returned value.
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

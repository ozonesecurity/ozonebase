#define DB_COMP_ID DB_COMP_ID_GENERAL

#include "libgenExec.h"

#include "libgenDebug.h"
#include "libgenConfig.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>

static SvrInitHandlersByPri    *initHandlers = 0;
static SvrInitHandlersByPri    *termHandlers = 0;

static SvrSigHandlersBySig     *sigHandlers = 0;

/**
* @brief 
*
* @param func
* @param priority
*
* @return 
*/
bool addSvrInitFunc( SvrInitHandler func, int priority )
{
    if ( !initHandlers )
        initHandlers = new SvrInitHandlersByPri;
    initHandlers->insert( SvrInitHandlersByPri::value_type( priority, func ) );
    return( true );
}

/**
* @brief 
*
* @return 
*/
bool svrInit()
{
    Info( "Running init functions" );
    srand( getpid() * time( 0 ) );
    if ( initHandlers )
    {
        for ( SvrInitHandlersByPri::iterator iter = initHandlers->begin(); iter != initHandlers->end(); iter++ )
            if ( !(iter->second)() )
                return( false );
    }
    return( true );
}

/**
* @brief 
*
* @param func
* @param priority
*
* @return 
*/
bool addSvrTermFunc( SvrInitHandler func, int priority )
{
    if ( !termHandlers )
        termHandlers = new SvrInitHandlersByPri;
    termHandlers->insert( SvrInitHandlersByPri::value_type( priority, func ) );
    return( true );
}

/**
* @brief 
*
* @return 
*/
bool svrTerm()
{
    Info( "Running term functions" );
    bool result = true;
    if ( termHandlers )
    {
        for ( SvrInitHandlersByPri::reverse_iterator iter = termHandlers->rbegin(); iter != termHandlers->rend(); iter++ )
            if ( !(iter->second)() )
                result = false;
        delete termHandlers;
        termHandlers = 0;
    }
    if ( sigHandlers )
    {
        for ( SvrSigHandlersBySig::iterator iter = sigHandlers->begin(); iter != sigHandlers->end(); iter++ )
        {
            delete iter->second;
        }
        delete sigHandlers;
        sigHandlers = 0;
    }
    delete initHandlers;
    initHandlers = 0;
    return( result );
}

/**
* @brief 
*
* @param signal
* @param func
* @param priority
*
* @return 
*/
bool addSvrSigHandlerFunc( int signal, SvrSigHandler func, int priority )
{
    if ( !sigHandlers )
        sigHandlers = new SvrSigHandlersBySig;

    switch( signal )
    {
        case SIGHUP :
        case SIGTERM :
        case SIGINT :
        {
            SvrSigHandlersBySig::iterator iter = sigHandlers->find( signal );
            if ( iter == sigHandlers->end() )
                iter = sigHandlers->insert( SvrSigHandlersBySig::value_type( signal, new SvrSigHandlersByPri ) );
            iter->second->insert( SvrSigHandlersByPri::value_type( priority, func ) );
            break;
        }
        default :
        {
            Error( "Attempt to add handler for signal %d (%s)", signal, strsignal(signal) );
        }
    }
    return( true );
}

/**
* @brief 
*
* @param signal
*/
static void hupHandler( int signal )
{
    Info( "Got HUP signal, looking for handlers" );
    if ( sigHandlers )
    {
        SvrSigHandlersBySig::iterator sigIter = sigHandlers->find( signal );
        if ( sigIter != sigHandlers->end() )
            for ( SvrSigHandlersByPri::iterator priIter = sigIter->second->begin(); priIter != sigIter->second->end(); priIter++ )
                (priIter->second)( signal );
    }
}

/**
* @brief 
*
* @param signal
*/
static void termHandler( int signal )
{
    Info( "Got TERM signal, looking for handlers" );
    if ( sigHandlers )
    {
        SvrSigHandlersBySig::iterator sigIter = sigHandlers->find( signal );
        if ( sigIter != sigHandlers->end() )
            for ( SvrSigHandlersByPri::iterator priIter = sigIter->second->begin(); priIter != sigIter->second->end(); priIter++ )
                (priIter->second)( signal );
    }
    //svrTerm();
    //exit( 0 );
}

/**
* @brief 
*
* @param signal
*/
static void intHandler( int signal )
{
    Info( "Got INT signal, looking for handlers" );
    if ( sigHandlers )
    {
        SvrSigHandlersBySig::iterator sigIter = sigHandlers->find( signal );
        if ( sigIter != sigHandlers->end() )
            for ( SvrSigHandlersByPri::iterator priIter = sigIter->second->begin(); priIter != sigIter->second->end(); priIter++ )
                (priIter->second)( signal );
    }
    //svrTerm();
    //exit( -1 );
}

#if HAVE_STRUCT_SIGCONTEXT
/**
* @brief 
*
* @param signal
* @param HAVE_SIGINFO_T && HAVE_UCONTEXT_T 
* @param info
* @param 
*/
static void dieHandler( int signal, struct sigcontext context )
#elif ( HAVE_SIGINFO_T && HAVE_UCONTEXT_T )
#include <ucontext.h>
/**
* @brief 
*
* @param signal
* @param info
* @param 
*/
static void dieHandler( int signal, siginfo_t *info, void *context )
#else
/**
* @brief 
*
* @param signal
*/
static void dieHandler( int signal )
#endif
{
#if HAVE_STRSIGNAL
    Error( "Got signal %d (%s), crashing", signal, strsignal(signal) );
#else // HAVE_STRSIGNAL
    Error( "Got signal %d, crashing", signal );
#endif // HAVE_STRSIGNAL

#ifndef ZM_NO_CRASHTRACE
#if ( ( HAVE_SIGINFO_T && HAVE_UCONTEXT_T ) || HAVE_STRUCT_SIGCONTEXT )
    void *trace[16];
    int trace_size = 0;

#if HAVE_STRUCT_SIGCONTEXT_EIP
    Error( "Signal address is %ld, from %ld", context.cr2, context.eip );

    trace_size = backtrace( trace, 16 );
    // overwrite sigaction with caller's address
    trace[1] = (void *)context.eip;
#elif HAVE_STRUCT_SIGCONTEXT
    Error( "Signal address is %p, no eip", context.cr2 );

    trace_size = backtrace( trace, 16 );
#else // HAVE_STRUCT_SIGCONTEXT
    if ( info && context )
    {
        ucontext_t *uc = (ucontext_t *)context;

        Error( "Signal address is %p, from %p", info->si_addr, uc->uc_mcontext.gregs[REG_EIP] );

        trace_size = backtrace( trace, 16 );
        // overwrite sigaction with caller's address
        trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
    }
#endif // HAVE_STRUCT_SIGCONTEXT
#if HAVE_DECL_BACKTRACE
    char **messages = (char **)NULL;

    messages = backtrace_symbols( trace, trace_size );
    // skip first stack frame (points here)
    for ( int i=1; i < trace_size; ++i )
        Error( "Backtrace: %s", messages[i] );
    Info( "Backtrace complete" );
#endif // HAVE_DECL_BACKTRACE
#endif // ( HAVE_SIGINFO_T && HAVE_UCONTEXT_T ) || HAVE_STRUCT_SIGCONTEXT
#endif // ZM_NO_CRASHTRACE

    exit( signal );
}

/**
* @brief 
*
* @return 
*/
static bool initSignalHandlers()
{
    Info( "Initialising Signal Handlers" );

    sigset_t block_set;
    sigemptyset( &block_set );
    struct sigaction action, old_action;

    action.sa_handler = (SvrSigHandler)hupHandler;
    action.sa_mask = block_set;
    action.sa_flags = 0;
    sigaction( SIGHUP, &action, &old_action );

    action.sa_handler = (SvrSigHandler)termHandler;
    action.sa_mask = block_set;
    action.sa_flags = 0;
    sigaction( SIGTERM, &action, &old_action );

    action.sa_handler = (SvrSigHandler)intHandler;
    action.sa_mask = block_set;
    action.sa_flags = 0;
    sigaction( SIGINT, &action, &old_action );

    if ( !gConfig->values.dumpCoreFiles )
    {
        action.sa_handler = (SvrSigHandler)dieHandler;
        action.sa_mask = block_set;
        action.sa_flags = 0;
        sigaction( SIGABRT, &action, &old_action );
        sigaction( SIGBUS, &action, &old_action );
        sigaction( SIGSEGV, &action, &old_action );
        sigaction( SIGILL, &action, &old_action );
        sigaction( SIGFPE, &action, &old_action );
    }
    return( true );
}

/**
* @brief 
*
* @return 
*/
static bool termSignalHandlers()
{
    Info( "Terminating Signal Handlers" );

    sigset_t block_set;
    sigemptyset( &block_set );
    struct sigaction action, old_action;

    action.sa_handler = SIG_DFL;
    action.sa_mask = block_set;
    action.sa_flags = 0;
    sigaction( SIGHUP, &action, &old_action );
    sigaction( SIGTERM, &action, &old_action );
    sigaction( SIGINT, &action, &old_action );
    if ( !gConfig->values.dumpCoreFiles )
    {
        sigaction( SIGABRT, &action, &old_action );
        sigaction( SIGBUS, &action, &old_action );
        sigaction( SIGSEGV, &action, &old_action );
        sigaction( SIGILL, &action, &old_action );
        sigaction( SIGFPE, &action, &old_action );
    }
    return( true );
}

/**
* @brief 
*/
SvrExecRegistration::SvrExecRegistration()
{
    addSvrInitFunc( initSignalHandlers );
    addSvrTermFunc( termSignalHandlers );
}

static SvrExecRegistration registration;

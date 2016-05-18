#ifndef ZM_SIGNAL_H
#define ZM_SIGNAL_H

#include "zm.h"

#include <signal.h>
#include <execinfo.h>

typedef RETSIGTYPE (SigHandler)( int );

extern bool gZmReload;
extern bool gZmTerminate;

RETSIGTYPE zmHupHandler( int signal );
RETSIGTYPE zmTermHandler( int signal );
#if HAVE_STRUCT_SIGCONTEXT
RETSIGTYPE zmDieHandler( int signal, struct sigcontext context );
#elif ( HAVE_SIGINFO_T && HAVE_UCONTEXT_T )
#include <ucontext.h>
RETSIGTYPE zmDieHandler( int signal, siginfo_t *info, void *context );
#else
RETSIGTYPE zmDieHandler( int signal );
#endif

void zmSetHupHandler( SigHandler *handler );
void zmSetTermHandler( SigHandler *handler );
void zmSetDieHandler( SigHandler *handler );

void zmSetDefaultHupHandler();
void zmSetDefaultTermHandler();
void zmSetDefaultDieHandler();

#endif // ZM_SIGNAL_H

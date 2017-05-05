#ifndef OZ_SIGNAL_H
#define OZ_SIGNAL_H

#include "oz.h"

#include <signal.h>
#include <execinfo.h>

typedef RETSIGTYPE (SigHandler)( int );

extern bool gZmReload;
extern bool gZmTerminate;

RETSIGTYPE ozHupHandler( int signal );
RETSIGTYPE ozTermHandler( int signal );
#if HAVE_STRUCT_SIGCONTEXT
RETSIGTYPE ozDieHandler( int signal, struct sigcontext context );
#elif ( HAVE_SIGINFO_T && HAVE_UCONTEXT_T )
#ifdef __APPLE__
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif
RETSIGTYPE ozDieHandler( int signal, siginfo_t *info, void *context );
#else
RETSIGTYPE ozDieHandler( int signal );
#endif

void ozSetHupHandler( SigHandler *handler );
void ozSetTermHandler( SigHandler *handler );
void ozSetDieHandler( SigHandler *handler );

void ozSetDefaultHupHandler();
void ozSetDefaultTermHandler();
void ozSetDefaultDieHandler();

#endif // OZ_SIGNAL_H

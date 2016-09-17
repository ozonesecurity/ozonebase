#ifndef LIBGEN_DEBUG_H
#define LIBGEN_DEBUG_H

#include <sys/types.h>  
#include <limits.h> 

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Leave 0 and below for debug */
#define DBG_INF         0
#define DBG_WAR         -1
#define DBG_ERR         -2
#define DBG_FAT         -3
#define DBG_PNC         -4

/* Define the level at which messages go through syslog */
#define DBG_SYSLOG      2

#define dbgPrintf(level,params...)  {\
                    if ((level) <= dbgLevel)\
                        dbgOutput( 0, __FILE__, __LINE__, level, ##params );\
                }

#define dbgHexdump(level,data,len)  {\
                    if ((level) <= dbgLevel)\
                        dbgOutput( 1, __FILE__, __LINE__, level, "%p (%lu)", data, (size_t)(len) );\
                }

/* Turn off debug here */
#ifndef DBG_OFF
#define Debug(level,params...)  dbgPrintf(level,##params)
#define Hexdump(level,data,len) dbgHexdump(level,data,len)
#else
#define Debug(level,params...)
#define Hexdump(level,data,len)
#endif

#define Info(params...)     dbgPrintf(DBG_INF,##params)
#define Warning(params...)  dbgPrintf(DBG_WAR,##params)
#define Error(params...)    dbgPrintf(DBG_ERR,##params)
#define Fatal(params...)    dbgPrintf(DBG_FAT,##params)
#define Panic(params...)    dbgPrintf(DBG_PNC,##params)
#define Mark()              Info("Mark/%s/%d",__FILE__,__LINE__)
#define Log()               Info("Log")
#ifdef __GNUC__
#define Enter(level)        dbgPrintf(level,("Entering %s",__PRETTY_FUNCTION__))
#define Exit(level)         dbgPrintf(level,("Exiting %s",__PRETTY_FUNCTION__))
#else
#if 0
#define Enter(level)        dbgPrintf(level,("Entering <unknown>"))
#define Exit(level)         dbgPrintf(level,("Exiting <unknown>"))
#endif
#define Enter(level)        
#define Exit(level)         
#endif

#ifdef __cplusplus
extern "C" {
#endif 

/* function declarations */
const char *dbgName();
void usrHandler( int sig );
int getDebugEnv( void );
int debugPrepareLog( void );
int debugInitialise( const char *name, const char *id, int level );
int debugReinitialise( const char *target );
int debugTerminate( void );
void dbgSubtractTime( struct timeval * const tp1, struct timeval * const tp2 );

#if defined(__STDC__) || defined(__cplusplus)
int dbgInit( const char *name, const char *id, int level );
int dbgReinit( const char *target );
int dbgTerm(void);
void dbgOutput( int hex, const char * const file, const int line, const int level, const char *fstring, ... ) __attribute__ ((format(printf, 5, 6)));
#else
int dbgInit();
int dbgReinit();
int dbgTerm();
void dbgOutput();
#endif

extern int dbgLevel;

#ifndef _STDIO_INCLUDED
#include <stdio.h>
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // LIBGEN_DEBUG_H

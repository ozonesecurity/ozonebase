#include "libgenDebug.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <signal.h>
#include <stdarg.h>
#ifdef __APPLE__
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif
#include <errno.h>
#include <stdexcept>

int dbgLevel = 0;

static char _dbgSyslog[64];
static char _dbgName[64];
static char _dbgId[64];

static char _dbgLog[PATH_MAX] = "";
static FILE *_dbgLogFP = (FILE *)NULL;
static int _dbgPrint = FALSE;
static int _dbgFlush = FALSE;
static int _dbgRuntime = FALSE;
static int _dbgAddLogId = FALSE;
static struct timeval _dbgStart;

static int _dbgRunning = FALSE;

const char *dbgName()
{
    return( _dbgName );
}

/**
* @brief 
*
* @param sig
*/
void usrHandler( int sig )
{
    if( sig == SIGUSR1)
    {
        if ( dbgLevel < 9 )
        {
            dbgLevel++;
        }
    }
    else if ( sig == SIGUSR2 )
    {
        if( dbgLevel > -3 )
        {
            dbgLevel--;
        }
    }
    Info( "Debug Level Changed to %d", dbgLevel );
}

/**
* @brief 
*
* @return 
*/
int getDebugEnv()
{
    char envName[128];
    char *envPtr = 0;

    envPtr = getenv( "DBG_PRINT" );
    if ( envPtr == (char *)NULL )
    {
        _dbgPrint = FALSE;
    }
    else
    {
        _dbgPrint = atoi( envPtr );
    }

    envPtr = getenv( "DBG_FLUSH" );
    if ( envPtr == (char *)NULL )
    {
        _dbgFlush = FALSE;
    }
    else
    {
        _dbgFlush = atoi( envPtr );
    }

    envPtr = getenv( "DBG_RUNTIME" );
    if ( envPtr == (char *)NULL )
    {
        _dbgRuntime = FALSE;
    }
    else
    {
        _dbgRuntime = atoi( envPtr );
    }

    envPtr = NULL;
    sprintf( envName, "DBG_LEVEL_%s_%s", _dbgName, _dbgId );
    envPtr = getenv(envName);
    if ( envPtr == (char *)NULL )
    {
        sprintf( envName, "DBG_LEVEL_%s", _dbgName );
        envPtr = getenv(envName);
        if ( envPtr == (char *)NULL )
        {
            sprintf( envName, "DBG_LEVEL" );
            envPtr = getenv(envName);
        }
    }
    if ( envPtr != (char *)NULL )
    {
        dbgLevel = atoi(envPtr);
    }

    envPtr = NULL;
    sprintf( envName, "DBG_LOG_%s_%s", _dbgName, _dbgId );
    envPtr = getenv(envName);
    if ( envPtr == (char *)NULL )
    {
        sprintf( envName, "DBG_LOG_%s", _dbgName );
        envPtr = getenv(envName);
        if ( envPtr == (char *)NULL )
        {
            sprintf( envName, "DBG_LOG" );
            envPtr = getenv(envName);
        }
    }
    if ( envPtr != (char *)NULL )
    {
        /* If we do not want to add a pid to the debug logs
         * which is the default, and original method
         */
        if ( envPtr[strlen(envPtr)-1] == '+' )
        {
            /* remove the + character from the string */
            envPtr[strlen(envPtr)-1] = '\0';
            _dbgAddLogId = TRUE;
        }
        if ( _dbgAddLogId == FALSE )
        {
            strncpy( _dbgLog, envPtr, sizeof(_dbgLog) );
        }
        else
        {
            snprintf( _dbgLog, sizeof(_dbgLog), "%s.%05d", envPtr, getpid() );
        }
    }

    return( 0 );
}

/**
* @brief 
*
* @return 
*/
int debugPrepareLog()
{
    FILE *tempLogFP = NULL;

    if ( _dbgLogFP )
    {
        fflush( _dbgLogFP );
        if ( fclose(_dbgLogFP) == -1 )
        {
            Error( "fclose(), error = %s",strerror(errno) );
            return( -1 );
        }
        _dbgLogFP = (FILE *)NULL;
    }

    if ( ( _dbgAddLogId == FALSE && _dbgLog[0] ) && ( _dbgLog[strlen(_dbgLog)-1] == '~' ) )
    {
        _dbgLog[strlen(_dbgLog)-1] = '\0';

        if ( (tempLogFP = fopen(_dbgLog, "r")) != NULL )
        {
            char oldLogPath[256];
            
            sprintf( oldLogPath, "%s.old", _dbgLog );
            rename( _dbgLog, oldLogPath );
            fclose( tempLogFP );
        }
    }

    if( _dbgLog[0] && (_dbgLogFP = fopen(_dbgLog,"w")) == (FILE *)NULL )
    {
        Error( "fopen() for %s, error = %s", _dbgLog, strerror(errno) );
        return( -1 );
    }
    return( 0 );
}

/**
* @brief 
*
* @param name
* @param id
* @param level
*
* @return 
*/
int debugInitialise( const char *name, const char *id, int level )
{
    int status;

    gettimeofday( &_dbgStart, NULL );

    strncpy( _dbgName, name, sizeof(_dbgName) );
    strncpy( _dbgId, id, sizeof(_dbgId) );
    dbgLevel = level;
    
    /* Now set up the syslog stuff */
    if ( _dbgId[0] )
        snprintf( _dbgSyslog, sizeof(_dbgSyslog), "%s_%s", _dbgName, _dbgId );
    else
        strncpy( _dbgSyslog, _dbgName, sizeof(_dbgSyslog) );

    (void) openlog( _dbgSyslog, LOG_PID|LOG_NDELAY, LOG_LOCAL1 );

    _dbgLogFP = (FILE *)NULL;

    if( (status = getDebugEnv() ) < 0)
    {
        Error( "Debug Environment Error, status = %d", status );
        return( -1 );
    }

    debugPrepareLog();

    Info( "Debug Level = %d, Debug Log = %s", dbgLevel, _dbgLog[0]?_dbgLog:"<none>" );

    {
    struct sigaction action;
    memset( &action, 0, sizeof(action) );
    action.sa_handler = usrHandler;
    action.sa_flags = SA_RESTART;

    if ( sigaction( SIGUSR1, &action, 0 ) < 0 )
    {
        Error( "sigaction(), error = %s", strerror(errno) );
        return( -1 );
    }
    if ( sigaction( SIGUSR2, &action, 0 ) < 0)
    {
        Error( "sigaction(), error = %s", strerror(errno) );
        return( -1 );
    }
    }
    _dbgRunning = TRUE;
    return( 0 );
}

/**
* @brief 
*
* @param name
* @param id
* @param level
*
* @return 
*/
int dbgInit( const char *name, const char *id, int level )
{
    return( debugInitialise( name, id, level ) );
}

/**
* @brief 
*
* @param target
*
* @return 
*/
int debugReinitialise( const char *target )
{
    int status;
    int reinit = FALSE;
    char buffer[64];

    if ( target )
    {
        snprintf( buffer, sizeof(buffer), "_%s_%s", _dbgName, _dbgId );
        if ( strcmp( target, buffer ) == 0 )
        {
            reinit = TRUE;
        }
        else
        {
            snprintf( buffer, sizeof(buffer), "_%s", _dbgName );
            if ( strcmp( target, buffer ) == 0 )
            {
                reinit = TRUE;
            }
            else
            {
                if ( strcmp( target, "" ) == 0 )
                {
                    reinit = TRUE;
                }
            }
        }
    }

    if ( reinit )
    {
        if ( (status = getDebugEnv() ) < 0 )
        {
            Error( "Debug Environment Error, status = %d", status );
            return( -1 );
        }

        debugPrepareLog();

        Info( "New Debug Level = %d, New Debug Log = %s", dbgLevel, _dbgLog[0]?_dbgLog:"<none>" );
    }

    return( 0 );
}

/**
* @brief 
*
* @param target
*
* @return 
*/
int dbgReinit( const char *target )
{
    return( debugReinitialise( target ) );
}

/**
* @brief 
*
* @return 
*/
int debugTerminate()
{
    Debug( 1, "Terminating Debug" );
    fflush( _dbgLogFP );
    if ( fclose(_dbgLogFP) == -1 )
    {
        Error( "fclose(), error = %s", strerror(errno) );
        return( -1 );
    }
    _dbgLogFP = (FILE *)NULL;
    (void) closelog();

    _dbgRunning = FALSE;
    return( 0 );
}

/**
* @brief 
*
* @return 
*/
int dbgTerm()
{
    return( debugTerminate() );
}

/**
* @brief 
*
* @param tp1
* @param tp2
*/
void dbgSubtractTime( struct timeval * const tp1, struct timeval * const tp2 )
{
    tp1->tv_sec -= tp2->tv_sec;
    if ( tp1->tv_usec <= tp2->tv_usec )
    {
        tp1->tv_sec--;
        tp1->tv_usec = 1000000 - (tp2->tv_usec - tp1->tv_usec);
    }
    else
    {
        tp1->tv_usec = tp1->tv_usec - tp2->tv_usec;
    }
}

/**
* @brief 
*
* @param hex
* @param file
* @param line
* @param level
* @param fstring
* @param ...
*/
void dbgOutput( int hex, const char * const file, const int line, const int level, const char *fstring, ... )
{
    char            classString[4];
    char            timeString[64];
    char            dbgString[8192];
    va_list         argPtr;
    int             logCode;
    struct timeval  timeVal;
    
    switch ( level )
    {
        case DBG_INF:
            strncpy( classString, "INF", sizeof(classString) );
            break;
        case DBG_WAR:
            strncpy( classString, "WAR", sizeof(classString) );
            break;
        case DBG_ERR:
            strncpy( classString, "ERR", sizeof(classString) );
            break;
        case DBG_FAT:
            strncpy( classString, "FAT", sizeof(classString) );
            break;
        case DBG_PNC:
            strncpy( classString, "PNC", sizeof(classString) );
            break;
        default:
            if ( level > 0 && level <= 9 )
            {
                snprintf( classString, sizeof(classString), "DB%d", level );
            }
            else
            {
                Error( "Unknown Error Level %d", level );
            }
            break;
    }

    gettimeofday( &timeVal, NULL );

    if ( _dbgRuntime )
    {
        dbgSubtractTime( &timeVal, &_dbgStart );

        snprintf( timeString, sizeof(timeString), "%ld.%03ld", timeVal.tv_sec, timeVal.tv_usec/1000 );
    }
    else
    {
        char *timePtr = timeString;
        timePtr += strftime( timePtr, sizeof(timeString), "%x %H:%M:%S", localtime(&timeVal.tv_sec) );
        snprintf( timePtr, sizeof(timeString)-(timePtr-timeString), ".%06ld", timeVal.tv_usec );
    }

    char *dbgPtr = dbgString;
    dbgPtr += snprintf( dbgPtr, sizeof(dbgString), "%s %s[%ld].%s-%s/%d [", 
                timeString,
                _dbgSyslog,
                syscall(SYS_gettid),
                classString,
                file,
                line
            );
    char *dbgLogStart = dbgPtr;

    va_start( argPtr, fstring );
    if ( hex )
    {
        unsigned char *data = va_arg( argPtr, unsigned char * );
        int len = va_arg( argPtr, int );
        int i;
        dbgPtr += snprintf( dbgPtr, sizeof(dbgString)-(dbgPtr-dbgString), "%d:", len );
        for ( i = 0; i < len; i++ )
        {
            int maxChars = sizeof(dbgString)-((dbgPtr-dbgString)+6);
            int printChars = snprintf( dbgPtr, maxChars, " %02x", data[i] );
            if ( printChars > maxChars )
            {
                dbgPtr += maxChars-1;
                strncpy( dbgPtr, "...", 4 );
                dbgPtr += 3;
                break;
            }
            dbgPtr += printChars;
        }
    }
    else
    {
        int maxChars = sizeof(dbgString)-((dbgPtr-dbgString)+6);
        int printChars = vsnprintf( dbgPtr, maxChars, fstring, argPtr );
        if ( printChars > maxChars )
        {
           dbgPtr += maxChars-1;
           strncpy( dbgPtr, "...", 4 );
           dbgPtr += 3;
        }
        else
           dbgPtr += printChars;
    }
    va_end(argPtr);
    char *dbg_log_end = dbgPtr;
    strncpy( dbgPtr, "]\n", sizeof(dbgString)-(dbgPtr-dbgString) );   

    if ( _dbgPrint )
    {
        printf( "%s", dbgString );
        fflush( stdout );
    }
    if ( _dbgLogFP != (FILE *)NULL )
    {
        fprintf( _dbgLogFP, "%s", dbgString );

        if ( _dbgFlush )
        {
            fflush( _dbgLogFP );
        }
    }
    /* For Info, Warning, Errors etc we want to log them */
    if ( level <= DBG_SYSLOG )
    {
        switch( level )
        {
            case DBG_INF:
                logCode = LOG_INFO;
                break;
            case DBG_WAR:
                logCode = LOG_WARNING;
                break;
            case DBG_ERR:
            case DBG_FAT:
            case DBG_PNC:
                logCode = LOG_ERR;
                break;
            default:
                logCode = LOG_DEBUG;
                break;
        }
        //logCode |= LOG_DAEMON;
        *dbg_log_end = '\0';
        syslog( logCode, "%s [%s]", classString, dbgLogStart );
    }
    if ( level <= DBG_FAT )
    {
        if ( level <= DBG_PNC )
            abort();
        // let's not exit, throw instead
        //exit( -1 );
        throw std::runtime_error(dbgString);
    }
}

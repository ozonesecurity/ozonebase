#include <stdlib.h>
#include <string.h>

#include "zm.h"
#include "zmDb.h"

MYSQL gDbConn;

void zmDbConnect()
{
    if ( !mysql_init( &gDbConn ) )
    {
        Error( "Can't initialise structure: %s", mysql_error( &gDbConn ) );
        exit( mysql_errno( &gDbConn ) );
    }
    std::string::size_type colonIndex = staticConfig.DB_HOST.find( ":/" );
    if ( colonIndex != std::string::npos )
    {
        std::string dbHost = staticConfig.DB_HOST.substr( 0, colonIndex );
        std::string dbPort = staticConfig.DB_HOST.substr( colonIndex+1 );
        if ( !mysql_real_connect( &gDbConn, dbHost.c_str(), staticConfig.DB_USER.c_str(), staticConfig.DB_PASS.c_str(), 0, atoi(dbPort.c_str()), 0, 0 ) )
        {
            Error( "Can't connect to server: %s", mysql_error( &gDbConn ) );
            exit( mysql_errno( &gDbConn ) );
        }
    }
    else
    {
        if ( !mysql_real_connect( &gDbConn, staticConfig.DB_HOST.c_str(), staticConfig.DB_USER.c_str(), staticConfig.DB_PASS.c_str(), 0, 0, 0, 0 ) )
        {
            Error( "Can't connect to server: %s", mysql_error( &gDbConn ) );
            exit( mysql_errno( &gDbConn ) );
        }
    }
    if ( mysql_select_db( &gDbConn, staticConfig.DB_NAME.c_str() ) )
    {
        Error( "Can't select database: %s", mysql_error( &gDbConn ) );
        exit( mysql_errno( &gDbConn ) );
    }
}


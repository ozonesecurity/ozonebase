#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "zm.h"

/**
* @brief 
*/
void zmLoadConfig()
{
    FILE *cfg;
    char line[512];
    char *val;
    if ( (cfg = fopen( ZM_CONFIG, "r")) == NULL )
    {
        Fatal("Can't open %s: %s", ZM_CONFIG, strerror(errno) );
    }
    while ( fgets( line, sizeof(line), cfg ) != NULL )
    {
        char *line_ptr = line;

        // Trim off any cr/lf line endings
        int chomp_len = strcspn( line_ptr, "\r\n" );
        line_ptr[chomp_len] = '\0';

        // Remove leading white space
        int white_len = strspn( line_ptr, " \t" );
        line_ptr += white_len;

        // Check for comment or empty line
        if ( *line_ptr == '\0' || *line_ptr == '#' )
            continue;

        // Remove trailing white space
        char *temp_ptr = line_ptr+strlen(line_ptr)-1;
        while ( *temp_ptr == ' ' || *temp_ptr == '\t' )
        {
            *temp_ptr-- = '\0';
            temp_ptr--;
        }

        // Now look for the '=' in the middle of the line
        temp_ptr = strchr( line_ptr, '=' );
        if ( !temp_ptr )
        {
            Warning( "Invalid data in %s: '%s'", ZM_CONFIG, line );
            continue;
        }

        // Assign the name and value parts
        char *name_ptr = line_ptr;
        char *val_ptr = temp_ptr+1;

        // Trim trailing space from the name part
        do
        {
            *temp_ptr = '\0';
            temp_ptr--;
        }
        while ( *temp_ptr == ' ' || *temp_ptr == '\t' );

        // Remove leading white space from the value part
        white_len = strspn( val_ptr, " \t" );
        val_ptr += white_len;

        val = (char *)malloc( strlen(val_ptr)+1 );
        strncpy( val, val_ptr, strlen(val_ptr)+1 );

        if ( strcasecmp( name_ptr, "ZM_DB_HOST" ) == 0 )
            staticConfig.DB_HOST = val;
        else if ( strcasecmp( name_ptr, "ZM_DB_NAME" ) == 0 )
            staticConfig.DB_NAME = val;
        else if ( strcasecmp( name_ptr, "ZM_DB_USER" ) == 0 )
            staticConfig.DB_USER = val;
        else if ( strcasecmp( name_ptr, "ZM_DB_PASS" ) == 0 )
            staticConfig.DB_PASS = val;
        else if ( strcasecmp( name_ptr, "ZM_PATH_WEB" ) == 0 )
            staticConfig.PATH_WEB = val;
        else
        {
            // We ignore this now as there may be more parameters than the
            // c/c++ binaries are bothered about
            // Warning( "Invalid parameter '%s' in %s", name_ptr, ZM_CONFIG );
        }
    }
    fclose( cfg);
    config.Assign();
}

StaticConfig staticConfig;

/**
* @brief 
*
* @param p_name
* @param p_value
* @param p_type
*/
ConfigItem::ConfigItem( const char *p_name, const char *p_value, const char *const p_type )
{
    name = new char[strlen(p_name)+1];
    strcpy( name, p_name );
    value = new char[strlen(p_value)+1];
    strcpy( value, p_value );
    type = new char[strlen(p_type)+1];
    strcpy( type, p_type );

    //Info( "Created new config item %s = %s (%s)\n", name, value, type );

    accessed = false;
}

/**
* @brief 
*/
ConfigItem::~ConfigItem()
{
    delete[] name;
    delete[] value;
    delete[] type;
}

/**
* @brief 
*/
void ConfigItem::ConvertValue() const
{
    if ( !strcmp( type, "boolean" ) )
    {
        cfg_type = CFG_BOOLEAN;
        cfg_value.boolean_value = (bool)strtol( value, 0, 0 );
    }
    else if ( !strcmp( type, "integer" ) )
    {
        cfg_type = CFG_INTEGER;
        cfg_value.integer_value = strtol( value, 0, 10 );
    }
    else if ( !strcmp( type, "hexadecimal" ) )
    {
        cfg_type = CFG_INTEGER;
        cfg_value.integer_value = strtol( value, 0, 16 );
    }
    else if ( !strcmp( type, "decimal" ) )
    {
        cfg_type = CFG_DECIMAL;
        cfg_value.decimal_value = strtod( value, 0 );
    }
    else
    {
        cfg_type = CFG_STRING;
        cfg_value.string_value = value;
    }
    accessed = true;
}

/**
* @brief 
*
* @return 
*/
bool ConfigItem::BooleanValue() const
{
    if ( !accessed )
        ConvertValue();

    if ( cfg_type != CFG_BOOLEAN )
    {
        Error( "Attempt to fetch boolean value for %s, actual type is %s. Try running 'zmupdate.pl -f' to reload config.", name, type );
        exit( -1 );
    }

    return( cfg_value.boolean_value );
}

/**
* @brief 
*
* @return 
*/
int ConfigItem::IntegerValue() const
{
    if ( !accessed )
        ConvertValue();

    if ( cfg_type != CFG_INTEGER )
    {
        Error( "Attempt to fetch integer value for %s, actual type is %s. Try running 'zmupdate.pl -f' to reload config.", name, type );
        exit( -1 );
    }

    return( cfg_value.integer_value );
}

/**
* @brief 
*
* @return 
*/
double ConfigItem::DecimalValue() const
{
    if ( !accessed )
        ConvertValue();

    if ( cfg_type != CFG_DECIMAL )
    {
        Error( "Attempt to fetch decimal value for %s, actual type is %s. Try running 'zmupdate.pl -f' to reload config.", name, type );
        exit( -1 );
    }

    return( cfg_value.decimal_value );
}

/**
* @brief 
*
* @return 
*/
const char *ConfigItem::StringValue() const
{
    if ( !accessed )
        ConvertValue();

    if ( cfg_type != CFG_STRING )
    {
        Error( "Attempt to fetch string value for %s, actual type is %s. Try running 'zmupdate.pl -f' to reload config.", name, type );
        exit( -1 );
    }

    return( cfg_value.string_value );
}

/**
* @brief 
*/
Config::Config()
{
    n_items = 0;
    items = 0;
}

/**
* @brief 
*/
Config::~Config()
{
    if ( items )
    {
        for ( int i = 0; i < n_items; i++ )
        {
            delete items[i];
        }
        delete[] items;
    }
}

/**
* @brief 
*/
void Config::Assign()
{
ZM_CFG_ASSIGN_LIST
}

/**
* @brief 
*
* @param id
*
* @return 
*/
const ConfigItem &Config::Item( int id )
{
    if ( !n_items )
    {
        Assign();
    }

    if ( id < 0 || id > ZM_MAX_CFG_ID )
    {
        Error( "Attempt to access invalid config, id = %d. Try running 'zmupdate.pl -f' to reload config.", id );
        exit( -1 );
    }

    ConfigItem *item = items[id];
    
    if ( !item )
    {
        Error( "Can't find config item %d", id );
        exit( -1 );
    }
        
    return( *item );
}

Config config;

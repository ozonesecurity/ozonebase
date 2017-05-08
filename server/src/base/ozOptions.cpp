#include "oz.h"
#include "ozOptions.h"

#include <unistd.h>
#include <iostream>
#include <fstream>

#ifdef __APPLE__
extern char **environ;
#endif

const Options gNullOptions;

unsigned int Options::load( const std::string &prefix, bool replace )
{
    char **envPtr = environ;
    int numOptions = 0;
    while ( *envPtr )
    {
        std::string env( *envPtr );
        std::string::size_type pos = env.find_first_of( "=", 1 );
        if ( pos != std::string::npos )
        {
            if  ( env.substr( 0, prefix.length() ) == prefix )
            {
                std::string name = env.substr( prefix.length(), pos-prefix.length() );
                if ( replace )
                    mOptionMap.erase( name );
                std::string value = env.substr( pos+1 );
                if ( value == "true" || value == "false" )
                {
                    add( name, value == "true" );
                }
                else if ( value.find_first_not_of( "0123456789." ) == std::string::npos )
                {
                    if ( value.find_first_of( "." ) == std::string::npos )
                    {
                        add( name, stoi(value) );
                    }
                    else
                    {
                        add( name, stod(value) );
                    }
                }
                else
                {
                    add( name, value );
                }
                numOptions++;
            }
        }
        envPtr++;
    }
    return( numOptions );
}

void Options::dump( unsigned int level ) const
{
    for ( OptionMap::const_iterator iter = mOptionMap.begin(); iter != mOptionMap.end(); iter++ )
    {
        //printf( "%s == %s\n", iter->second->type().c_str(), typeid(*this).name() );
        if ( iter->second->type() == typeid(*this).name() )
        {
            std::cout << std::string(level+1,'.') << iter->first << " (" << iter->second->type() << ") => " << std::endl;
            //printf( "%*s%s (%s) =>\n", level*2, "", iter->first.c_str(), iter->second->type().c_str() );
            _Option *optionPtr = iter->second.get();
            Option<Options> *optionsPtr = dynamic_cast<Option<Options> *>(optionPtr);
            optionsPtr->value().dump( level+1 );
        }
        else
        {
            std::cout << std::string(level+1,'.') << iter->first << " (" << iter->second->type() << ") => " <<  iter->second->valueString() << std::endl;
            //printf( "%*s%s (%s) => %s\n", level*2, "", iter->first.c_str(), iter->second->type().c_str(), iter->second->valueString().c_str() );
        }
    }
}

void to_json( json &j, const Options &o )
{
	o.jsonDump( j );
}

void Options::jsonDump( json &j ) const
{
	for ( OptionMap::const_iterator iter = mOptionMap.begin(); iter != mOptionMap.end(); iter++ )
	{
		iter->second->jsonDump( j, iter->first );
	}
}

void from_json( const json &j, Options &o )
{
    // Not used
}

void Options::jsonLoad( const json &j )
{
	for ( json::const_iterator iter = j.begin(); iter != j.end(); iter++ )
	{
		printf( "%s = ", iter.key().c_str() );
		if ( iter->is_null() )
		{
			printf( "null" );
		}
		if ( iter->is_boolean() )
		{
			printf( "bool" );
            add( iter.key(), iter->get<bool>() );
		}
		else if ( iter->is_number() )
		{
			printf( "number_" );
		    if ( iter->is_number_integer() )
		    {
			    printf( "integer" );
                add( iter.key(), iter->get<int>() );
		    }
		    else if ( iter->is_number_unsigned() )
		    {
			    printf( "unsigned" );
                add( iter.key(), iter->get<unsigned int>() );
		    }
		    else if ( iter->is_number_float() )
		    {
			    printf( "float" );
                add( iter.key(), iter->get<double>() );
            }
            else
            {
			    printf( "unknown" );
            }
		}
		else if ( iter->is_string() )
		{
			printf( "string/%s", iter->get<std::string>().c_str() );
            add( iter.key(), iter->get<std::string>() );
		}
		else if ( iter->is_object() )
		{
			printf( "object" );
            Options options;
            options.jsonLoad( *iter );
            add( iter.key(), options );

		}
		else if ( iter->is_array() )
		{
			printf( "array" );
		}
        else
        {
			printf( "unknown" );
		}
		printf( "\n" );
		//iter->second->jsonDump( j, iter->first );
	}
}

bool Options::readFile( const std::string &filename )
{
	try {
        std::ifstream file;
        file.exceptions ( std::ifstream::badbit );
        file.open( filename );
	    json j;
	    file >> j;
	    jsonLoad( j );
    }
    catch ( const std::exception &e )
    {
        Error( "Unable to read options from file '%s': %s (%s)", filename.c_str(), e.what(), std::strerror(errno) );
        return( false );
    }
    return( true );
}

bool Options::writeFile( const std::string &filename, bool pretty )
{
	try {
        std::ofstream file;
        file.exceptions ( std::ofstream::failbit | std::ofstream::badbit );
        file.open( filename );
	    json j;
	    jsonDump( j );
        if ( pretty )
            file << std::setw(2) << j << std::endl;
        else
            file << j << std::endl;
    }
    catch ( const std::exception &e )
    {
        Error( "Unable to write options to file '%s': %s (%s)", filename.c_str(), e.what(), std::strerror(errno) );
        return( false );
    }
    return( true );
}

#include "oz.h"
#include "ozOptions.h"

#include <unistd.h>
#include <iostream>

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

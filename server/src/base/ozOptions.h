#ifndef OZ_OPTIONS_H
#define OZ_OPTIONS_H

#include <memory>
#include <map>
#include <sstream>
#include <unistd.h>

#ifdef __APPLE__
#include <cstdlib>
#endif

#include "../libgen/libgenUtils.h"
#include "../libgen/libgenException.h"


#include "../../../externals/json/src/json.hpp"

// for convenience
using json = nlohmann::json;

// Base class for option
class _Option
{
protected:
    _Option() {
    }

public:
    virtual ~_Option() 
    {
    }
    virtual const std::string type() const
    {
        static const std::string typeString = "null";
        return( typeString );
    }
    virtual const std::string valueString() const
    {
        static const std::string valString = "(null)";
        return( valString );
    }
    virtual const void jsonDump( json &j, const std::string &name ) const
    {
        j[name] = nullptr;
    }
};

template <class t> class Option : public _Option
{
private:
    t mValue;

public:
    Option<t>( const t &value ) :
        mValue( value )
    {
    }

    const t &value() const
    {
        return( mValue );
    }
    const std::string valueString() const
    {
        std::ostringstream valString;
        valString << mValue;
        return( valString.str() );
    }
    const std::string type() const
    {
        return typeid(t).name();
    }
    const void jsonDump( json &j, const std::string &name ) const
    {
        j[name] = mValue;
    }
};

class Options : public _Option
{
public:
    typedef std::map<const std::string, std::shared_ptr<_Option>> OptionMap;

private:
    OptionMap   mOptionMap;

public:
    Options() {
    }

    unsigned int load( const std::string &prefix="OZ_OPT_", bool replace=false );
    void dump( unsigned int depth=0 ) const;

    bool readFile( const std::string &filename );
    bool writeFile( const std::string &filename, bool pretty=false );

    bool exists( const std::string &name )
    {
        OptionMap::const_iterator iter = mOptionMap.find( name );
        return( iter != mOptionMap.end() );
    }
    bool add( const std::string &name, const char *value )
    {
        return( add( name, std::string(value) ) );
    }
    bool add( const std::string &name, char value[] )
    {
        return( add( name, std::string(value) ) );
    }
    template <typename t> bool add( const std::string &name, t value )
    {
        std::pair<OptionMap::iterator,bool> result = mOptionMap.insert( OptionMap::value_type( name, std::make_shared<Option<t>>( value ) ) );
        return( result.second );
    }
    bool remove( const std::string &name )
    {
        return( mOptionMap.erase( name ) == 1 );
    }
    void clear()
    {
        mOptionMap.clear();
    }

    bool set( const std::string &name, const char *value )
    {
        return( set( name, std::string(value) ) );
    }
    bool set( const std::string &name, char value[] )
    {
        return( set( name, std::string(value) ) );
    }
    template <typename t> bool set( const std::string &name, t value )
    {
        OptionMap::const_iterator iter = mOptionMap.find( name );
        bool found = ( iter != mOptionMap.end() );
        if ( found )
            remove( name );
        add( name, value );
        return( !found );
    }

    template <typename t> const std::pair<const t&,OptionMap::const_iterator> _get( const std::string &name, const t &notFoundValue )
    {
        OptionMap::const_iterator iter = mOptionMap.find( name );
        bool found = ( iter != mOptionMap.end() );
        if ( !found )
            return( std::pair<const t&,OptionMap::const_iterator>(notFoundValue,iter) );
        std::shared_ptr<Option<t>> optionPtr = std::dynamic_pointer_cast<Option<t>>( iter->second );
        Option<t> *option = optionPtr.get();
        if ( !option )
            throw Exception( stringtf( "Option %s found, but of wrong type. Expected %s, got %s.", name.c_str(), typeid(t).name(), iter->second->type().c_str() ) );
        return( std::pair<const t&,OptionMap::const_iterator>(option->value(),iter) );
    }
    const std::string &get( const std::string &name, const char *&notFoundValue )
    {
        //std::unique_ptr<std::string> temp( new std::string(notFoundValue) );
        return( get( name, std::string(notFoundValue) ) );
    }
    const std::string &get( const std::string &name, const char notFoundValue[] )
    {
        //std::unique_ptr<std::string> temp( new std::string(notFoundValue) );
        return( get( name, std::string(notFoundValue) ) );
    }
    template <typename t> const t &get( const std::string &name, const t &notFoundValue )
    {
        const std::pair<const t&,OptionMap::const_iterator> result = _get( name, notFoundValue );
        return( result.first );
    }
    const std::string &extract( const std::string &name, const char *notFoundValue )
    {
        return( extract( name, std::string(notFoundValue) ) );
    }
    template <typename t> const t &extract( const std::string &name, const t &notFoundValue )
    {
        const std::pair<const t&,OptionMap::const_iterator> result = _get( name, notFoundValue );
        if ( result.second != mOptionMap.end() )
            mOptionMap.erase( result.second );
        return( result.first );
    }

    friend std::ostream& operator<< (std::ostream& stream, const Options& options )
    {
        stream << "options";
        return ( stream );
    }

    friend void to_json( json &j, const Options &o );
    void jsonDump( json &j ) const;

    friend void from_json( const json &j, Options &o );
    void jsonLoad( const json &j );
};

extern const Options gNullOptions;

#endif // OZ_OPTIONS_H

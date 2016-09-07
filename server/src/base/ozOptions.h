#ifndef OZ_OPTIONS_H
#define OZ_OPTIONS_H

#include <memory>
#include <map>

#include "../libgen/libgenUtils.h"
#include "../libgen/libgenException.h"

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

    //bool add( const std::string &name, const Option *value )
    //{
        //std::pair<OptionMap::iterator,bool> result = mOptionMap.insert( OptionMap::value_type( name, option ) );
        //return( result.second );
    //}
    template <typename t> bool add( const std::string &name, t value )
    {
        std::pair<OptionMap::iterator,bool> result = mOptionMap.insert( OptionMap::value_type( name, std::make_shared<Option<t>>( value ) ) );
        return( result.second );
    }
    bool add( const std::string &name, const char *value )
    {
        std::pair<OptionMap::iterator,bool> result = mOptionMap.insert( OptionMap::value_type( name, std::make_shared<Option<const std::string>>( value ) ) );
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
    /*
    const Option &get( const std::string &name )
    {
        OptionMap::const_iterator iter = mOptionMap.find( name );
        bool result = ( iter != mOptionMap.end() );
        if ( !result )
            return( gNullOption );
        return( iter->second )
    }
    */
    template <typename t> const std::pair<const t&,OptionMap::const_iterator> _get( const std::string &name, const t &notFoundValue )
    {
        OptionMap::const_iterator iter = mOptionMap.find( name );
        bool result = ( iter != mOptionMap.end() );
        if ( !result )
            return( std::pair<const t&,OptionMap::const_iterator>(notFoundValue,iter) );
        std::shared_ptr<Option<t>> optionPtr = std::dynamic_pointer_cast<Option<t>>( iter->second );
        Option<t> *option = optionPtr.get();
        if ( !option )
            throw Exception( stringtf( "Option %s found, but of wrong type", name.c_str() ) );
        return( std::pair<const t&,OptionMap::const_iterator>(option->value(),iter) );
    }
    template <typename t> const t &get( const std::string &name, const t &notFoundValue )
    {
        const std::pair<const t&,OptionMap::const_iterator> result = _get( name, notFoundValue );
        return( result.first );
    }
    const std::string &get( const std::string &name, const char *notFoundValue )
    {
        const std::pair<const std::string&,OptionMap::const_iterator> result = _get( name, std::string(notFoundValue) );
        return( result.first );
    }
    template <typename t> const t &extract( const std::string &name, const t &notFoundValue )
    {
        const std::pair<const t&,OptionMap::const_iterator> result = _get( name, notFoundValue );
        if ( result.second != mOptionMap.end() )
            mOptionMap.erase( result.second );
        return( result.first );
    }
    const std::string &extract( const std::string &name, const char *notFoundValue )
    {
        const std::pair<const std::string&,OptionMap::const_iterator> result = _get( name, std::string(notFoundValue) );
        if ( result.second != mOptionMap.end() )
            mOptionMap.erase( result.second );
        return( result.first );
    }
};

#endif // OZ_OPTIONS_H

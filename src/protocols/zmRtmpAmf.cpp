#include "../zm.h"
#include "zmRtmpAmf.h"

#include "../libgen/libgenException.h"
#include "../libgen/libgenUtils.h"

Amf0Record::Amf0Record( const Amf0Object &object ) :
    mType( OBJECT_MARKER ),
    mOpen( false )
{
    mValue.pObject = new Amf0Object( object );
}

Amf0Record::Amf0Record( const Amf0Array &array ) :
    mType( ARRAY_MARKER ),
    mOpen( false )
{
    mValue.pArray = new Amf0Array( array );
}

Amf0Record::Amf0Record( ByteBuffer &buffer ) :
    mOpen( false )
{
    mType = (Amf0DataType)buffer--;
    switch( mType )
    {
        case NUMBER_MARKER :
        {
            uint64_t rawValue;
            memcpy( &rawValue, buffer.data(), sizeof(rawValue) );
            uint64_t convValue = be64toh( rawValue );
#if defined(__arm__)
            uint64_t swapValue;
            memcpy( &swapValue, (uint8_t *)&convValue+sizeof(uint32_t), sizeof(uint32_t) );
            memcpy( (uint8_t *)&swapValue+sizeof(uint32_t), &convValue, sizeof(uint32_t) );
            convValue = swapValue;
#endif
            memcpy( &mValue.number, &convValue, sizeof(mValue.number) );
            Debug( 5, "Got number %lf", mValue.number );
            buffer -= sizeof(rawValue);
            break;
        }
        case BOOLEAN_MARKER :
        {
            mValue.boolean = buffer--;
            Debug( 5, "Got boolean %d", mValue.boolean );
            break;
        }
        case STRING_MARKER :
        {
            uint16_t length;
            memcpy( &length, buffer.data(), sizeof(length) );
            length = be16toh( length );
            buffer -= sizeof(length);
            if  ( length )
            {
                mValue.pString = new std::string( reinterpret_cast<char *>(buffer.data()), length );
                Debug( 5, "Got string %s, length %d", mValue.pString->c_str(), length );
                buffer -= length;
            }
            else
            {
                mValue.pString = new std::string;
                Debug( 5, "Got null string" );
            }
            break;
        }
        case OBJECT_MARKER :
        {
            mValue.pObject = new Amf0Object;
            mOpen = true;
            Debug( 5, "Opening object" );
            while ( mOpen && buffer.size() )
            {
                uint16_t length;
                memcpy( &length, buffer.data(), sizeof(length) );
                length = be16toh( length );
                buffer -= sizeof(length);
                if ( length == 0 )
                {
                    Debug( 5, "Got zero length, expecting object close" );
                }
                if ( buffer[0] == OBJECT_END_MARKER )
                {
                    Debug( 5, "Closing object" );
                    mOpen = false;
                    buffer--;
                    break;
                }
                std::string name( reinterpret_cast<char *>(buffer.data()), length );
                buffer -= length;
                Debug( 5, "Field name %s", name.c_str() );
                Amf0Record valueRecord( buffer );
                mValue.pObject->addRecord( name, valueRecord );
            }
            break;
        }
        case ARRAY_MARKER :
        {
            mValue.pArray = new Amf0Array;
            mOpen = true;
            Debug( 5, "Opening array" );
            while ( mOpen && buffer.size() )
            {
                uint16_t length;
                memcpy( &length, buffer.data(), sizeof(length) );
                length = be16toh( length );
                buffer -= sizeof(length);
                if ( length == 0 )
                {
                    Debug( 5, "Got zero length, expecting object close" );
                }
                if ( buffer[0] == OBJECT_END_MARKER )
                {
                    Debug( 5, "Closing array" );
                    mOpen = false;
                    buffer--;
                    break;
                }
                std::string name( reinterpret_cast<char *>(buffer.data()), length );
                buffer -= length;
                Debug( 5, "Field name %s", name.c_str() );
                Amf0Record valueRecord( buffer );
                mValue.pArray->addRecord( name, valueRecord );
            }
            break;
        }
        case NULL_MARKER :
        {
            Debug( 5, "Got null marker" );
            break;
        }
        case UNDEFINED_MARKER :
        {
            Debug( 5, "Got undefined marker" );
            break;
        }
        default :
        {
            throw( Exception( stringtf( "Unexpected AMF0 Data Type %d", mType ) ) );
        }
    }
}

Amf0Record::Amf0Record( const Amf0Record &record )
{
    memset( &mValue, 0, sizeof(mValue) );
    *this = record;
}

Amf0Record::~Amf0Record()
{
    switch ( mType )
    {
        case STRING_MARKER :
            delete mValue.pString;
            break;
        case OBJECT_MARKER :
            delete mValue.pObject;
            break;
        case ARRAY_MARKER :
            delete mValue.pArray;
            break;
    }
}

bool Amf0Record::addRecord( const std::string &name, const Amf0Record &record )
{
    if ( !isContainer() )
        Panic( "Attempt to add AMF0 field to non-container type" );
    if ( !isOpen() )
        Panic( "Attempt to add AMF0 field to non-open container" );
    switch ( mType )
    {
        case OBJECT_MARKER :
            return( mValue.pObject->addRecord( name, record ) );
            break;
        case ARRAY_MARKER :
            return( mValue.pArray->addRecord( name, record ) );
            break;
        default :
            Panic( "Unexpected AMF0 data type %d in addField", mType );
    }
    return( false );
}

Amf0Record &Amf0Record::operator=( const Amf0Record &record )
{
    mType = record.mType;
    switch ( mType )
    {
        case STRING_MARKER :
            delete mValue.pString;
            mValue.pString = new std::string(*record.mValue.pString);
            break;
        case OBJECT_MARKER :
            delete mValue.pObject;
            mValue.pObject = new Amf0Object(*record.mValue.pObject);
            break;
        case ARRAY_MARKER :
            delete mValue.pArray;
            mValue.pArray = new Amf0Array(*record.mValue.pArray);
            break;
        default :
            memcpy( &mValue, &record.mValue, sizeof(mValue) );
            break;
    }
    mOpen = record.mOpen;
    return( *this );
}

size_t Amf0Record::encode( ByteBuffer &buffer ) const
{
    size_t initialSize = buffer.size();
    switch( mType )
    {
        case NUMBER_MARKER :
        {
            buffer.append( mType );
            uint64_t convValue;
            memcpy( &convValue, &mValue.number, sizeof(convValue) );
#if defined(__arm__)
            uint64_t swapValue;
            memcpy( &swapValue, (uint8_t *)&convValue+sizeof(uint32_t), sizeof(uint32_t) );
            memcpy( (uint8_t *)&swapValue+sizeof(uint32_t), &convValue, sizeof(uint32_t) );
            convValue = swapValue;
#endif
            uint64_t rawValue = htobe64( convValue );
            buffer.append( &rawValue, sizeof(rawValue) );
            break;
        }
        case BOOLEAN_MARKER :
        {
            buffer.append( mType );
            buffer.append( mValue.boolean );
            break;
        }
        case STRING_MARKER :
        {
            buffer.append( mType );
            uint16_t length = htobe16(mValue.pString->length());
            buffer.append( &length, sizeof(length) );
            if  ( mValue.pString->length() )
                buffer.append( mValue.pString->data(), mValue.pString->length() );
            break;
        }
        case OBJECT_MARKER :
        {
            mValue.pObject->encode( buffer );
            break;
        }
        case ARRAY_MARKER :
        {
            mValue.pArray->encode( buffer );
            break;
        }
        case NULL_MARKER :
        {
            buffer.append( mType );
            break;
        }
        default :
        {
            throw( Exception( stringtf( "Unexpected AMF0 Data Type %d", mType ) ) );
        }
    }
    return( buffer.size() - initialSize );
}

bool Amf0Object::addRecord( const std::string &name, const Amf0Record &record )
{
    //pair<RecordMap::iterator,bool> result = mRecords.insert( RecordMap::value_type( name, record ) );
    //return( result.second );
    mRecords.push_back( NameRecord( name, record ) );
    std::pair<RecordIndex::iterator,bool> result = mRecordIndex.insert( RecordIndex::value_type( name, record ) );
    return( result.second );
}

const Amf0Record *Amf0Object::getRecord( const std::string &name ) const
{
    RecordIndex::const_iterator iter = mRecordIndex.find( name );
    if ( iter == mRecordIndex.end() )
        return( false );
    return( &(iter->second) );
}

size_t Amf0Object::encode( ByteBuffer &buffer ) const
{
    size_t initialSize = buffer.size();
    buffer.append( Amf0Record::OBJECT_MARKER );
    //for ( RecordMap::const_iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
    for ( RecordList::const_iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
    {
        const std::string &name = (*iter).name;
        uint16_t length = htobe16(name.length());
        buffer.append( &length, sizeof(length) );
        buffer.append( name.data(), name.length() );
        const Amf0Record &value = (*iter).record;
        value.encode( buffer );
    }
    buffer.append( 0 );
    buffer.append( 0 );
    buffer.append( Amf0Record::OBJECT_END_MARKER );
    return( buffer.size() - initialSize );
}

bool Amf0Array::addRecord( const std::string &name, const Amf0Record &record )
{
    //pair<RecordMap::iterator,bool> result = mRecords.insert( RecordMap::value_type( name, record ) );
    //return( result.second );
    mRecords.push_back( NameRecord( name, record ) );
    return( true );
}

size_t Amf0Array::encode( ByteBuffer &buffer ) const
{
    size_t initialSize = buffer.size();
    buffer.append( Amf0Record::ARRAY_MARKER );
    buffer.append( 0 );
    buffer.append( 0 );
    buffer.append( 0 );
    buffer.append( 0 );
    //for ( RecordMap::const_iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
    for ( RecordList::const_iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
    {
        const std::string &name = (*iter).name;
        uint16_t length = htobe16(name.length());
        buffer.append( &length, sizeof(length) );
        buffer.append( name.data(), name.length() );
        const Amf0Record &value = (*iter).record;
        value.encode( buffer );
    }
    buffer.append( 0 );
    buffer.append( 0 );
    buffer.append( Amf0Record::OBJECT_END_MARKER );
    return( buffer.size() - initialSize );
}

Amf3Record::Amf3Record( const Amf3Object &object ) :
    mType( OBJECT_MARKER ),
    mOpen( false )
{
    mValue.pObject = new Amf3Object( object );
}

Amf3Record::Amf3Record( const Amf3Array &array ) :
    mType( ARRAY_MARKER ),
    mOpen( false )
{
    mValue.pArray = new Amf3Array( array );
}

Amf3Record::Amf3Record( ByteBuffer &buffer ) :
    mOpen( false )
{
    mType = (Amf3DataType)buffer--;
    switch( mType )
    {
        case INTEGER_MARKER :
        {
            int32_t rawValue;
            memcpy( &rawValue, buffer.data(), sizeof(rawValue) );
            int32_t convValue = be32toh( rawValue );
            memcpy( &mValue.integer, &convValue, sizeof(mValue.integer) );
            Debug( 5, "Got integer %d", mValue.integer );
            buffer -= sizeof(rawValue);
            break;
        }
        case DOUBLE_MARKER :
        {
            uint64_t rawValue;
            memcpy( &rawValue, buffer.data(), sizeof(rawValue) );
            uint64_t convValue = be64toh( rawValue );
            memcpy( &mValue.double_, &convValue, sizeof(mValue.double_) );
            Debug( 5, "Got double %lf", mValue.double_ );
            buffer -= sizeof(rawValue);
            break;
        }
        case STRING_MARKER :
        {
            uint16_t length;
            memcpy( &length, buffer.data(), sizeof(length) );
            length = be16toh( length );
            buffer -= sizeof(length);
            if  ( length )
            {
                mValue.pString = new std::string( reinterpret_cast<char *>(buffer.data()), length );
                Debug( 5, "Got string %s, length %d", mValue.pString->c_str(), length );
                buffer -= length;
            }
            else
            {
                mValue.pString = new std::string;
                Debug( 5, "Got null string" );
            }
            break;
        }
        case OBJECT_MARKER :
        {
#if 0
            mValue.pObject = new Amf3Object;
            mOpen = true;
            while ( mOpen && buffer.size() )
            {
                uint16_t length;
                memcpy( &length, buffer.data(), sizeof(length) );
                length = be16toh( length );
                buffer -= sizeof(length);
                if ( length == 0 )
                {
                    Debug( 5, "Got zero length, expecting object close" );
                }
                if ( buffer[0] == OBJECT_END_MARKER )
                {
                    Debug( 5, "Closing object" );
                    mOpen = false;
                    buffer--;
                    break;
                }
                std::string name( reinterpret_cast<char *>(buffer.data()), length );
                buffer -= length;
                Debug( 5, "Field name %s", name.c_str() );
                Amf3Record valueRecord( buffer );
                mValue.pObject->addRecord( name, valueRecord );
            }
#endif
            break;
        }
        case ARRAY_MARKER :
        {
#if 0
            mValue.pArray = new Amf3Array;
            mOpen = true;
            while ( mOpen && buffer.size() )
            {
                uint16_t length;
                memcpy( &length, buffer.data(), sizeof(length) );
                length = be16toh( length );
                buffer -= sizeof(length);
                if ( length == 0 )
                {
                    Debug( 5, "Got zero length, expecting object close" );
                }
                if ( buffer[0] == OBJECT_END_MARKER )
                {
                    Debug( 5, "Closing array" );
                    mOpen = false;
                    buffer--;
                    break;
                }
                std::string name( reinterpret_cast<char *>(buffer.data()), length );
                buffer -= length;
                Debug( 5, "Field name %s", name.c_str() );
                Amf3Record valueRecord( buffer );
                mValue.pArray->addRecord( name, valueRecord );
            }
#endif
            break;
        }
        case NULL_MARKER :
        {
            Debug( 5, "Got null marker" );
            break;
        }
        default :
        {
            throw( Exception( stringtf( "Unexpected AMF3 Data Type %d", mType ) ) );
        }
    }
}

Amf3Record::Amf3Record( const Amf3Record &record )
{
    memset( &mValue, 0, sizeof(mValue) );
    *this = record;
}

Amf3Record::~Amf3Record()
{
    switch ( mType )
    {
        case STRING_MARKER :
            delete mValue.pString;
            break;
        case OBJECT_MARKER :
            delete mValue.pObject;
            break;
        case ARRAY_MARKER :
            delete mValue.pArray;
            break;
    }
}

bool Amf3Record::addRecord( const std::string &name, const Amf3Record &record )
{
    if ( !isContainer() )
        Panic( "Attempt to add AMF3 field to non-container type" );
    if ( !isOpen() )
        Panic( "Attempt to add AMF3 field to non-open container" );
    switch ( mType )
    {
        case OBJECT_MARKER :
            return( mValue.pObject->addRecord( name, record ) );
            break;
        case ARRAY_MARKER :
            return( mValue.pArray->addRecord( name, record ) );
            break;
        default :
            Panic( "Unexpected AMF3 data type %d in addField", mType );
    }
    return( false );
}

Amf3Record &Amf3Record::operator=( const Amf3Record &record )
{
    mType = record.mType;
    switch ( mType )
    {
        case STRING_MARKER :
            delete mValue.pString;
            mValue.pString = new std::string(*record.mValue.pString);
            break;
        case OBJECT_MARKER :
            delete mValue.pObject;
            mValue.pObject = new Amf3Object(*record.mValue.pObject);
            break;
        case ARRAY_MARKER :
            delete mValue.pArray;
            mValue.pArray = new Amf3Array(*record.mValue.pArray);
            break;
        default :
            memcpy( &mValue, &record.mValue, sizeof(mValue) );
            break;
    }
    mOpen = record.mOpen;
    return( *this );
}

size_t Amf3Record::encode( ByteBuffer &buffer ) const
{
    size_t initialSize = buffer.size();
    switch( mType )
    {
        case INTEGER_MARKER :
        {
            buffer.append( mType );
            int32_t convValue;
            memcpy( &convValue, &mValue.integer, sizeof(convValue) );
            int32_t rawValue = htobe32( convValue );
            buffer.append( &rawValue, sizeof(rawValue) );
            break;
        }
        case DOUBLE_MARKER :
        {
            buffer.append( mType );
            uint64_t convValue;
            memcpy( &convValue, &mValue.double_, sizeof(convValue) );
            uint64_t rawValue = htobe64( convValue );
            buffer.append( &rawValue, sizeof(rawValue) );
            break;
        }
        case STRING_MARKER :
        {
            buffer.append( mType );
            uint16_t length = htobe16(mValue.pString->length());
            buffer.append( &length, sizeof(length) );
            if  ( mValue.pString->length() )
                buffer.append( mValue.pString->data(), mValue.pString->length() );
            break;
        }
        case OBJECT_MARKER :
        {
            mValue.pObject->encode( buffer );
            break;
        }
        case ARRAY_MARKER :
        {
            mValue.pArray->encode( buffer );
            break;
        }
        case NULL_MARKER :
        {
            buffer.append( mType );
            break;
        }
        default :
        {
            throw( Exception( stringtf( "Unexpected AMF3 Data Type %d", mType ) ) );
        }
    }
    return( buffer.size() - initialSize );
}

bool Amf3Object::addRecord( const std::string &name, const Amf3Record &record )
{
    //pair<RecordMap::iterator,bool> result = mRecords.insert( RecordMap::value_type( name, record ) );
    //return( result.second );
    mRecords.push_back( NameRecord( name, record ) );
    return( true );
}

size_t Amf3Object::encode( ByteBuffer &buffer ) const
{
#if 0
    size_t initialSize = buffer.size();
    buffer.append( Amf3Record::OBJECT_MARKER );
    //for ( RecordMap::const_iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
    for ( RecordList::const_iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
    {
        const std::string &name = (*iter).name;
        uint16_t length = htobe16(name.length());
        buffer.append( &length, sizeof(length) );
        buffer.append( name.data(), name.length() );
        const Amf3Record &value = (*iter).record;
        value.encode( buffer );
    }
    buffer.append( 0 );
    buffer.append( 0 );
    buffer.append( Amf3Record::OBJECT_END_MARKER );
    return( buffer.size() - initialSize );
#endif
}

bool Amf3Array::addRecord( const std::string &name, const Amf3Record &record )
{
    //pair<RecordMap::iterator,bool> result = mRecords.insert( RecordMap::value_type( name, record ) );
    //return( result.second );
    mRecords.push_back( NameRecord( name, record ) );
    return( true );
}

size_t Amf3Array::encode( ByteBuffer &buffer ) const
{
#if 0
    size_t initialSize = buffer.size();
    buffer.append( Amf3Record::ARRAY_MARKER );
    buffer.append( 0 );
    buffer.append( 0 );
    buffer.append( 0 );
    buffer.append( 0 );
    //for ( RecordMap::const_iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
    for ( RecordList::const_iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
    {
        const std::string &name = (*iter).name;
        uint16_t length = htobe16(name.length());
        buffer.append( &length, sizeof(length) );
        buffer.append( name.data(), name.length() );
        const Amf3Record &value = (*iter).record;
        value.encode( buffer );
    }
    buffer.append( 0 );
    buffer.append( 0 );
    buffer.append( Amf3Record::OBJECT_END_MARKER );
    return( buffer.size() - initialSize );
#endif
}


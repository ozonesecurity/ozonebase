#ifndef ZM_RTMP_AMF_H
#define ZM_RTMP_AMF_H

#include "../libgen/libgenBuffer.h"

#include <string>
#include <map>
#include <vector>
#include <utility>

#include <stdint.h>

class Amf0Object;
class Amf0Array;

class Amf0Record
{
friend class Amf0Object;

public:
    typedef enum {
        NUMBER_MARKER,
        BOOLEAN_MARKER,
        STRING_MARKER,
        OBJECT_MARKER,
        MOVIECLIP_MARKER,
        NULL_MARKER,
        UNDEFINED_MARKER,
        REFERENCE_MARKER,
        ARRAY_MARKER,
        OBJECT_END_MARKER,
        STRICT_ARRAY_MARKER,
        DATE_MARKER,
        LONG_STRING_MARKER,
        UNSUPPORTED_MARKER,
        RECORDSET_MARKER,
        XML_DOCUMENT_MARKER,
        TYPED_OBJECT_MARKER
    } Amf0DataType;

private:
    Amf0DataType    mType;
    union {
        double      number;
        bool        boolean;
        std::string *pString;
        Amf0Object  *pObject;
        Amf0Array  *pArray;
    } mValue;
    bool mOpen;

public:
    Amf0Record() :
        mType( NULL_MARKER ),
        mOpen( false )
    {
    }
    Amf0Record( double value ) :
        mType( NUMBER_MARKER ),
        mOpen( false )
    {
        mValue.number = value;
    }
    Amf0Record( int value ) :
        mType( NUMBER_MARKER ),
        mOpen( false )
    {
        mValue.number = value;
    }
    Amf0Record( bool value ) :
        mType( BOOLEAN_MARKER ),
        mOpen( false )
    {
        mValue.boolean = value;
    }
    Amf0Record( const char *value ) :
        mType( STRING_MARKER ),
        mOpen( false )
    {
        mValue.pString = new std::string(value);
    }
    Amf0Record( const std::string &value ) :
        mType( STRING_MARKER ),
        mOpen( false )
    {
        mValue.pString = new std::string(value);
    }
    Amf0Record( const Amf0Object &object );
    Amf0Record( const Amf0Array &array );
    Amf0Record( ByteBuffer &buffer );
    Amf0Record( const Amf0Record &record );
    ~Amf0Record();

    Amf0Record &operator=( const Amf0Record &record );

    Amf0DataType type() const
    {
        return( mType );
    }
    int getInt() const
    {
        if ( mType != NUMBER_MARKER )
            Panic( "Attempt to get int from non number AMF0 record type %d", mType );
        return( (int)(mValue.number) );
    }
    double getDouble() const
    {
        if ( mType != NUMBER_MARKER )
            Panic( "Attempt to get double from non number AMF0 record type %d", mType );
        return( (mValue.number) );
    }
    bool getBoolean() const
    {
        if ( mType != BOOLEAN_MARKER )
            Panic( "Attempt to get boolean from non boolean AMF0 record type %d", mType );
        return( mValue.boolean );
    }
    const std::string &getString() const
    {
        if ( mType != STRING_MARKER )
            Panic( "Attempt to get string from non string AMF0 record type %d", mType );
        return( *(mValue.pString) );
    }
    const Amf0Object &getObject() const
    {
        if ( mType != OBJECT_MARKER )
            Panic( "Attempt to get object from non object AMF0 record type %d", mType );
        return( *(mValue.pObject) );
    }
    const Amf0Array &getArray() const
    {
        if ( mType != ARRAY_MARKER )
            Panic( "Attempt to get object from non array AMF0 record type %d", mType );
        return( *(mValue.pArray) );
    }

    bool isContainer() const
    {
        return( mType == OBJECT_MARKER || mType == ARRAY_MARKER );
    }
    bool isOpen() const
    {
        return( mOpen );
    }

    bool addRecord( const std::string &name, const Amf0Record &record );

    size_t encode( ByteBuffer &buffer ) const;
};

class Amf0Object
{
private:
    //typedef std::map<std::string,Amf0Record> RecordMap;
    struct NameRecord {
        std::string name;
        Amf0Record record;
        NameRecord( const std::string &pName, const Amf0Record &pRecord ) :
            name( pName ),
            record( pRecord )
        {
        }
    };
    typedef std::vector<NameRecord> RecordList;
    typedef std::map<const std::string,const Amf0Record> RecordIndex;

private:
    RecordList   mRecords;
    RecordIndex  mRecordIndex;

public:
    Amf0Object()
    {
    }
    Amf0Object( const Amf0Object &object )
    {
        *this = object;
    }
    ~Amf0Object()
    {
        //for ( RecordList::iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
        //{
            //delete iter->second;
        //}
    }

    Amf0Object &operator=( const Amf0Object &object )
    {
        mRecords = object.mRecords;
        mRecordIndex = object.mRecordIndex;
        return( *this );
    }

    bool addRecord( const std::string &name, const Amf0Record &record );
    const Amf0Record *getRecord( const std::string &name ) const;

    size_t encode( ByteBuffer &buffer ) const;
};

class Amf0Array
{
private:
    //typedef std::map<std::string,Amf0Record> RecordMap;
    struct NameRecord {
        std::string name;
        Amf0Record record;
        NameRecord( const std::string &pName, const Amf0Record &pRecord ) :
            name( pName ),
            record( pRecord )
        {
        }
    };
    typedef std::vector<NameRecord> RecordList;

private:
    RecordList   mRecords;

public:
    Amf0Array()
    {
    }
    Amf0Array( const Amf0Array &array )
    {
        *this = array;
    }
    ~Amf0Array()
    {
        //for ( RecordMap::iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
        for ( RecordList::iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
        {
            //delete iter->second;
        }
    }

    Amf0Array &operator=( const Amf0Array &array )
    {
        mRecords = array.mRecords;
        return( *this );
    }

    bool addRecord( const std::string &name, const Amf0Record &record );

    size_t encode( ByteBuffer &buffer ) const;
};

class Amf3Object;
class Amf3Array;

class Amf3Record
{
friend class Amf3Object;

public:
    typedef enum {
        UNDEFINED_MARKER,
        NULL_MARKER,
        FALSE_MARKER,
        TRUE_MARKER,
        INTEGER_MARKER,
        DOUBLE_MARKER,
        STRING_MARKER,
        XML_DOC_MARKER,
        DATE_MARKER,
        ARRAY_MARKER,
        OBJECT_MARKER,
        XML_MARKER,
        BYTE_ARRAY_MARKER,
    } Amf3DataType;

private:
    Amf3DataType    mType;
    union {
        int32_t     integer;
        double      double_;
        bool        boolean;
        std::string *pString;
        Amf3Object  *pObject;
        Amf3Array  *pArray;
    } mValue;
    bool mOpen;

public:
    Amf3Record() :
        mType( NULL_MARKER ),
        mOpen( false )
    {
    }
    Amf3Record( int value ) :
        mType( INTEGER_MARKER ),
        mOpen( false )
    {
        mValue.integer = value;
    }
    Amf3Record( double value ) :
        mType( DOUBLE_MARKER ),
        mOpen( false )
    {
        mValue.double_ = value;
    }
    Amf3Record( const char *value ) :
        mType( STRING_MARKER ),
        mOpen( false )
    {
        mValue.pString = new std::string(value);
    }
    Amf3Record( const std::string &value ) :
        mType( STRING_MARKER ),
        mOpen( false )
    {
        mValue.pString = new std::string(value);
    }
    Amf3Record( const Amf3Object &object );
    Amf3Record( const Amf3Array &array );
    Amf3Record( ByteBuffer &buffer );
    Amf3Record( const Amf3Record &record );
    ~Amf3Record();

    Amf3Record &operator=( const Amf3Record &record );

    Amf3DataType type() const
    {
        return( mType );
    }
    int getInt() const
    {
        if ( mType != INTEGER_MARKER )
            Panic( "Attempt to get int from non integer AMF3 record type %d", mType );
        return( (int)(mValue.integer) );
    }
    double getDouble() const
    {
        if ( mType != DOUBLE_MARKER )
            Panic( "Attempt to get double from non double AMF3 record type %d", mType );
        return( (mValue.double_) );
    }
    const std::string &getString() const
    {
        if ( mType != STRING_MARKER )
            Panic( "Attempt to get string from non string AMF3 record type %d", mType );
        return( *(mValue.pString) );
    }
    const Amf3Object &getObject() const
    {
        if ( mType != OBJECT_MARKER )
            Panic( "Attempt to get object from non object AMF3 record type %d", mType );
        return( *(mValue.pObject) );
    }
    const Amf3Array &getArray() const
    {
        if ( mType != ARRAY_MARKER )
            Panic( "Attempt to get object from non array AMF3 record type %d", mType );
        return( *(mValue.pArray) );
    }

    bool isContainer() const
    {
        return( mType == OBJECT_MARKER || mType == ARRAY_MARKER );
    }
    bool isOpen() const
    {
        return( mOpen );
    }

    bool addRecord( const std::string &name, const Amf3Record &record );

    size_t encode( ByteBuffer &buffer ) const;
};

class Amf3Object
{
private:
    //typedef std::map<std::string,Amf3Record> RecordMap;
    struct NameRecord {
        std::string name;
        Amf3Record record;
        NameRecord( const std::string &pName, const Amf3Record &pRecord ) :
            name( pName ),
            record( pRecord )
        {
        }
    };
    typedef std::vector<NameRecord> RecordList;

private:
    RecordList   mRecords;

public:
    Amf3Object()
    {
    }
    Amf3Object( const Amf3Object &object )
    {
        *this = object;
    }
    ~Amf3Object()
    {
        //for ( RecordMap::iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
        for ( RecordList::iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
        {
            //delete iter->second;
        }
    }

    Amf3Object &operator=( const Amf3Object &object )
    {
        mRecords = object.mRecords;
        return( *this );
    }

    bool addRecord( const std::string &name, const Amf3Record &record );

    size_t encode( ByteBuffer &buffer ) const;
};

class Amf3Array
{
private:
    //typedef std::map<std::string,Amf3Record> RecordMap;
    struct NameRecord {
        std::string name;
        Amf3Record record;
        NameRecord( const std::string &pName, const Amf3Record &pRecord ) :
            name( pName ),
            record( pRecord )
        {
        }
    };
    typedef std::vector<NameRecord> RecordList;

private:
    RecordList   mRecords;

public:
    Amf3Array()
    {
    }
    Amf3Array( const Amf3Array &array )
    {
        *this = array;
    }
    ~Amf3Array()
    {
        //for ( RecordMap::iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
        for ( RecordList::iterator iter = mRecords.begin(); iter != mRecords.end(); iter++ )
        {
            //delete iter->second;
        }
    }

    Amf3Array &operator=( const Amf3Array &array )
    {
        mRecords = array.mRecords;
        return( *this );
    }

    bool addRecord( const std::string &name, const Amf3Record &record );

    size_t encode( ByteBuffer &buffer ) const;
};

#endif // ZM_RTMP_AMF_H

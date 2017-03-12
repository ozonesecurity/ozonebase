/** @addtogroup Utilities */
/*@{*/
#ifndef OZ_NOTIFY_FRAME_H
#define OZ_NOTIFY_FRAME_H

#include "ozFeedFrame.h"
#include "../libgen/libgenTime.h"

///
/// Specialised data frame that is used to signal notifications around the system
///
class NotifyFrame : public DataFrame
{
protected:
    typedef enum { ADVISORY, IMPORTANT, CRITICAL } Urgency;

protected:
    Urgency mUrgency;      ///< Indicates the urgency of this notification

public:
    NotifyFrame( FeedProvider *provider, uint64_t id, uint64_t timestamp, Urgency urgency ) :
        DataFrame( provider, id, timestamp ),
        mUrgency( urgency )
    {
    }
    NotifyFrame( FeedProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, Urgency urgency ) :
        DataFrame( provider, parent, id, timestamp ),
        mUrgency( urgency )
    {
    }
    NotifyFrame( FeedProvider *provider, const FramePtr &parent, Urgency urgency ) :
        DataFrame( provider, parent ),
        mUrgency( urgency )
    {
    }

    Urgency urgency() const { return( mUrgency ); }
};

///
/// Notification frame for general disk I/O
///
class DiskIONotification : public NotifyFrame
{
public:
    class DiskIODetail {
        public:
            typedef enum { READ, WRITE, APPEND } IOType;
            typedef enum { BEGIN, PROGRESS, COMPLETE } IOStage;
            typedef enum { ERROR=-1, UNKNOWN, SUCCESS } IOResult;

        protected:
            IOType          mType;      ///< Type of IO operation
            IOStage         mStage;     ///< Stage of the IO operation, normally complete
            std::string     mName;      ///< Name of file/device involved
            size_t          mSize;      ///< Number of bytes read/written
            IOResult        mResult;    ///< Overall result of the operation
            int             mError;     ///< Reported error code if any

        public:
            //! General constructor
            DiskIODetail( IOType type, IOStage stage, const std::string name, size_t size=0, IOResult result=SUCCESS, int error=0 ) :
                mType( type ),
                mStage( stage ),
                mName( name ),
                mSize( size ),
                mResult( result ),
                mError( error )
            {
            }

            //! Shortcut constructor for successful write
            DiskIODetail( const std::string name, size_t size ) :
                mType( WRITE ),
                mStage( COMPLETE ),
                mName( name ),
                mSize( size ),
                mResult( SUCCESS ),
                mError( 0 )
            {
            }
            IOType type() const { return( mType ); }
            IOStage stage() const { return( mStage ); }
            const std::string &name() const { return( mName ); }
            size_t size() const { return( mSize ); }
            IOResult result() const { return( mResult ); }
            bool success() const { return( mResult == SUCCESS ); }
            bool failure() const { return( mResult == ERROR ); }
            int error() const { return( mError ); }
    };

protected:
    DiskIODetail    mDetail;        ///< Details of the I/O operation

public:
    //! General constructors
    DiskIONotification( FeedProvider *provider, uint64_t id, uint64_t timestamp, const DiskIODetail &detail ) :
        NotifyFrame( provider, id, timestamp, detail.failure()?IMPORTANT:ADVISORY ),
        mDetail( detail )
    {
    }
    DiskIONotification( FeedProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const DiskIODetail &detail ) :
        NotifyFrame( provider, parent, id, timestamp, detail.failure()?IMPORTANT:ADVISORY ),
        mDetail( detail )
    {
    }
    DiskIONotification( FeedProvider *provider, const FramePtr &parent, const DiskIODetail &detail ) :
        NotifyFrame( provider, parent, detail.failure()?IMPORTANT:ADVISORY ),
        mDetail( detail )
    {
    }

    //! Shortcut constructors
    DiskIONotification( FeedProvider *provider, uint64_t id, const DiskIODetail &detail ) :
        NotifyFrame( provider, id, time64(), detail.failure()?IMPORTANT:ADVISORY ),
        mDetail( detail )
    {
    }
    DiskIONotification( FeedProvider *provider, uint64_t id, const std::string name, size_t size ) :
        NotifyFrame( provider, id, time64(), ADVISORY ),
        mDetail( name, size )
    {
    }

    const DiskIODetail &detail() const { return( mDetail ); }
};

///
/// Notification frame for motion or other 'events'
///
class EventNotification : public NotifyFrame
{
public:
    class EventDetail {
        public:
            typedef enum { BEGIN, PROGRESS, COMPLETE } Stage;

        protected:
            ozId_t          mId;        ///< Event id
            Stage           mStage;     ///< Stage of the event operation
            double          mLength;    ///< Length of the event in seconds

        public:
            //! General constructor
            EventDetail( ozId_t id, Stage stage, double length=0.0 ) :
                mId( id ),
                mStage( stage ),
                mLength( length )
            {
            }

            //! Shortcut constructor for event complete
            EventDetail( ozId_t id, double length ) :
                mId( id ),
                mStage( COMPLETE ),
                mLength( length )
            {
            }
            ozId_t id() const { return( mId ); }
            Stage stage() const { return( mStage ); }
            double length() const { return( mLength ); }
    };

protected:
    EventDetail    mDetail;        ///< Details of the I/O operation

public:
    //! General constructors
    EventNotification( FeedProvider *provider, uint64_t id, uint64_t timestamp, const EventDetail &detail ) :
        NotifyFrame( provider, id, timestamp, ADVISORY ),
        mDetail( detail )
    {
    }
    EventNotification( FeedProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const EventDetail &detail ) :
        NotifyFrame( provider, parent, id, timestamp, ADVISORY ),
        mDetail( detail )
    {
    }
    EventNotification( FeedProvider *provider, const FramePtr &parent, const EventDetail &detail ) :
        NotifyFrame( provider, parent, ADVISORY ),
        mDetail( detail )
    {
    }

    //! Shortcut constructors
    EventNotification( FeedProvider *provider, uint64_t id, const EventDetail &detail ) :
        NotifyFrame( provider, id, time64(), ADVISORY ),
        mDetail( detail )
    {
    }

    const EventDetail &detail() const { return( mDetail ); }
};

#endif // OZ_NOTIFY_FRAME_H
/*@}*/

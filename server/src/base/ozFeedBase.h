#ifndef OZ_FEED_BASE_H
#define OZ_FEED_BASE_H

#include "countedPtr.h"
#include "linkedPtr.h"
#include "../libgen/libgenDebug.h"
#include <string>
#include <list>

typedef uint64_t ozId_t;

class FeedFrame;
///
/// Garbage collected pointer to frames.
/// Frames are passed around continuously from providers to consumers and onwards. This method allows frames to be
/// automatically freed once they are no longer being referenced by any other component
///
//typedef counted_ptr<const FeedFrame>    FramePtr;
typedef linked_ptr<const FeedFrame>    FramePtr;

#define CLASSID(className) \
public: \
    static const char *cClass() \
    { \
        return( #className ); \
    } \
    static std::string Class() \
    { \
        return( #className ); \
    } \
private: \
    void includeCLASSID() const {}

///
/// Base class for main classes that have tags and identities.
///
class FeedBase
{
protected:
    static const int INTERFRAME_TIMEOUT = 10000; ///< Needs to be short enough to cope with all possible frame rates.This allows for 100fps

private:
    std::string     mIdentity;          ///< Readable string combining the below

protected:
    std::string     mClass;             ///< Identifier for the class of provider/consumer
    std::string     mName;              ///< Unique identifier of this provider/consumer instance

public:
    /// Build identity from the class and name strings
    static inline std::string makeIdentity( const std::string &cl4ss, const std::string &name )
    {
        return( cl4ss+":"+name );
    }

private:
    virtual void includeCLASSID() const=0;     ///< Hack to ensure that all concrete classes include CLASSID macro

protected:
    FeedBase() 
    {
    }
    /// Identify this instance 
    void setIdentity( const std::string &cl4ss, const std::string &name )
    {
        if ( !mIdentity.empty() && (cl4ss != mClass || name != mName) )
            Fatal( "Relabelling %s -> %s:%s", mIdentity.c_str(), cl4ss.c_str(), name.c_str() );
        mClass = cl4ss;
        mName = name;
        mIdentity = makeIdentity( mClass, mName );
        //Debug( 1, "Identified %s (%s/%s)", mIdentity.c_str(), mClass.c_str(), mName.c_str() );
    }

public:
    const std::string &cl4ss() const { return( mClass ); }          ///< Return the class string
    const char *cclass() const { return( mClass.c_str() ); }        ///< Return the class string as a c-style string
    const std::string &name() const { return( mName ); }            ///< Return the name string
    const char *cname() const { return( mName.c_str() ); }          ///< Return the name string as a c-style string
    const std::string &identity() const { return( mIdentity ); }    ///< Return the identity string
    const char *cidentity() const { return( mIdentity.c_str() ); }  ///< Return the identity string as a c-style string
};

class FeedConsumer;
typedef bool (*FeedComparator)( const FramePtr &, const FeedConsumer * );   ///< Typedef for functions that are used to run boolean operations on frames

typedef enum { FEED_QUEUED=0x01, FEED_POLLED=0x02 } FeedLinkType;   ///< The nature of the relationship between provider and consumer
typedef std::list<FeedComparator>  FeedComparatorList;

///
/// Class representing the relationship between a provider and consumer.
///
class FeedLink
{
private:
    FeedLinkType        mLinkType;                                  ///< Whether the link is queued, polled or both?
    FeedComparatorList  mComparators;                               ///< What, if any, comparators to check 

public:
    FeedLink( FeedLinkType linkType, FeedComparator comparator=NULL ) :
        mLinkType( linkType )
    {
        if ( !linkType )
            Fatal( "No link type specified" );
        if ( comparator )
            mComparators.push_back( comparator );
    }
    FeedLink( FeedLinkType linkType, FeedComparatorList comparators ) :
        mLinkType( linkType ),
        mComparators( comparators )
    {
        if ( !linkType )
            Fatal( "No link type specified" );
    }
    FeedLink( const FeedLink &feedLink ) :
        mLinkType( feedLink.mLinkType ),
        mComparators( feedLink.mComparators )
    {
    }
    void setPolled() { mLinkType = (FeedLinkType)(mLinkType|FEED_POLLED); }
    void clearPolled() { mLinkType = (FeedLinkType)(mLinkType&~FEED_POLLED); }
    bool isPolled() const { return( mLinkType & FEED_POLLED ); }
    void setQueued() { mLinkType = (FeedLinkType)(mLinkType|FEED_QUEUED); }
    void clearQueued() { mLinkType = (FeedLinkType)(mLinkType&~FEED_QUEUED); }
    bool isQueued() const { return( mLinkType & FEED_QUEUED ); }
    const FeedComparatorList &comparators() const { return( mComparators ); }
    bool hasComparators() const { return( !mComparators.empty() ); }
    bool compare( const FramePtr &, const FeedConsumer * ) const;

    FeedLink &operator=( const FeedLink &feedLink )
    {
        mLinkType = feedLink.mLinkType;
        mComparators = feedLink.mComparators;
        return( *this );
    }
};

/// Some standard link types
extern FeedLink     gQueuedFeedLink;
extern FeedLink     gQueuedVideoLink;
extern FeedLink     gQueuedAudioLink;
extern FeedLink     gQueuedDataLink;

extern FeedLink     gPolledFeedLink;
extern FeedLink     gPolledVideoLink;
extern FeedLink     gPolledAudioLink;
extern FeedLink     gPolledDataLink;

#endif // OZ_FEED_BASE_H

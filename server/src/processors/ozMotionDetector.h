/** @addtogroup Processors */
/*@{*/

#ifndef OZ_MOTION_DETECTOR_H
#define OZ_MOTION_DETECTOR_H

#include "../base/ozDetector.h"
#include "../base/ozZone.h"
#include "../base/ozOptions.h"

#include <set>
#include <map>
#include <list>

#define SIGNAL_CAUSE "Signal"
#define MOTION_CAUSE "Motion"
#define LINKED_CAUSE "Linked"

class MotionData;

///
/// Processor that detects motion on a video frame feed and reports on the nature of the motion via
/// MotionFrame objects.
///
class MotionDetector : public virtual Detector
{
CLASSID(MotionDetector);

private:
    typedef std::set<std::string> StringSet;
    typedef std::map<std::string,StringSet> StringSetMap;
    typedef std::set<Zone *> ZoneSet;
    typedef std::list<Zone *> ZoneList;

private:
    Options         mOptions;

    Zone::Config    mZoneDefaults;          // Default zone config

    int             mFastStart;             // Whether to progressively blend images until the full blend ratios reached

    bool            mBlendAlarmedImages;

    int             mAnalysisScale;         // How much to scale down the difference image before detection
    int             mRefBlend;              // Inverse ratio new image going into reference image.
    int             mVarBlend;              // Inverse ratio new image going into variance buffer.
    int             mVarDeltaShift;         // Shift derived from above
    bool            mColourDeltas;          // Whether to compare image colours as opposed to just the Y brighness channel
    bool            mDeltaCorrection;       // Whether to apply general level correction before comparing images

    uint8_t         mNoiseLevel;            // Base level of noise in images
    uint32_t        mNoiseLevelSq;          // Base level of noise in images, squared and shifted to long fixed point

    //Image           mImage;
    Image           mRefImage;
    Uint32Buffer    mVarBuffer;
    Image           *mVarImage;

    int             mAlarmCount;

    int             mImageCount;
    int             mReadyCount;
    int             mFirstAlarmCount;
    int             mLastAlarmCount;
    bool            mAlarmed;
    time_t          mStartTime;

    ZoneSet         mZones;

    bool            mDebugStreams;
    bool            mDebugImages;
    std::string     mDebugLocation;

    SlaveVideo      *mCompImageSlave;
    SlaveVideo      *mRefImageSlave;
    SlaveVideo      *mDeltaImageSlave;
    SlaveVideo      *mVarImageSlave;

private:
    void construct();

public:
    MotionDetector( const std::string &name, const Options &options=gNullOptions );
    MotionDetector( VideoProvider &provider, const Options &options=gNullOptions, const FeedLink &link=gQueuedVideoLink );
    ~MotionDetector();

    bool addZone( Zone *zone );
    uint16_t width() const { return( videoProvider()->width() ); }
    uint16_t height() const { return( videoProvider()->height() ); }
    AVPixelFormat pixelFormat() const { return( Image::getNativePixelFormat( videoProvider()->pixelFormat() ) ); }
    FrameRate frameRate() const { return( videoProvider()->frameRate() ); }

    VideoProvider *compImageSlave() const { return( mCompImageSlave ); }
    VideoProvider *refImageSlave() const { return( mRefImageSlave ); }
    VideoProvider *deltaImageSlave() const { return( mDeltaImageSlave ); }
    VideoProvider *varImageSlave() const { return( mVarImageSlave ); }

protected:
    int run();

protected:
    uint32_t detectMotion( const Image &compImage, ZoneSet &motionZones );
    bool analyse( const Image *image, MotionData *motionData, Image *motionImage );
    bool alarmed() const { return( mAlarmed ); }

public:
    static bool inAlarm( const FramePtr &, const FeedConsumer * );
    static bool notInAlarm( const FramePtr &, const FeedConsumer * );
};

#endif // OZ_MOTION_DETECTOR_H


/*@}*/

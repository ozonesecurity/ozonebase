/** @addtogroup Consumers */
/*@{*/

#ifndef OZ_VIDEO_RECORDER_H
#define OZ_VIDEO_RECORDER_H

#include "../base/ozFeedConsumer.h"
#include "../libgen/libgenThread.h"

#include "../base/ozMotionFrame.h"
#include "../base/ozParameters.h"
#include "../base/ozRecorder.h"
#include <deque>


///
/// Consumer class used to record video stream for which motion or other significant
/// activity has been detected. Ultimately will write to a DB, but for now just to filesystem.
///
class VideoRecorder : public virtual Recorder
{
CLASSID(VideoRecorder);

protected:
    static const int MAX_EVENT_HEAD_AGE = 3;    ///< Number of seconds of video before event to save
    static const int MAX_EVENT_TAIL_AGE = 3;    ///< Number of seconds of video after event to save

protected:
    typedef std::deque<FramePtr> FrameStore;
    typedef enum { IDLE, PREALARM, ALARM, ALERT } AlarmState;

protected:
    std::string     mLocation;                  ///< Where to store the saved files
    std::string     mFormat;                    ///< Video file format/extension
    VideoParms      mVideoParms;                ///< Video parameters
    AudioParms      mAudioParms;                ///< Audio parameters

    FrameStore      mFrameStore;
    AlarmState      mState;
    int             mEventCount;                ///< Temp, in lieu of DB event index
    int             mNotificationId;
    uint64_t        mAlarmTime;

    AVFormatContext *mOutputContext;
    struct SwsContext *mConvertContext;
    AVDictionary    *mEncodeOpts;
    AVStream        *mVideoStream;
    AVStream        *mAudioStream;
    AVFrame         *mAvInputFrame;
    AVFrame         *mAvInterFrame;
    ByteBuffer      mInterBuffer;
    ByteBuffer      mVideoBuffer;
    ByteBuffer      mAudioBuffer;

    int             mVideoFrameCount;
    int             mAudioFrameCount;
    double          mMinTime;

protected:
    int run();
    bool processFrame( const FramePtr & );
    void initEncoder(); 
    void deinitEncoder(); 
    void openVideoFile( const std::string &filename );
    void closeVideoFile();
    void encodeFrame( const VideoFrame *frame );

public:
/**
* @brief 
*
* @param name Name of instance
* @param location Base path where to store videos
* @param format "mp4" and other filetypes. Use "mp4" for now
* @param videoParms VideoParms object that has been created
* @param audioParms AudioParms object that has been created
* @param minTime Minimum amount of time (in seconds) to record after an alarm is triggered

\code{.cpp}
    VideoParms* videoParms= new VideoParms( video_record_w, video_record_h );
    AudioParms* audioParms = new AudioParms;
    nvrcam.event = new VideoRecorder(name, path, "mp4", *videoParms, *audioParms, 30);
\endcode

*/
    VideoRecorder( const std::string &name, const std::string &location, const std::string &format, const VideoParms &videoParms, const AudioParms &audioParms , double minTime = 0) :
        VideoConsumer( cClass(), name, 5 ),
        Thread( identity() ),
        mLocation( location ),
        mFormat( format ),
        mVideoParms( videoParms ),
        mAudioParms( audioParms ),
        mState( IDLE ),
        mEventCount( 0 ),
        mNotificationId( 0 ),
        mAlarmTime( 0 ),
        mOutputContext( NULL ),
        mConvertContext( NULL ),
        mEncodeOpts( NULL ),
        mVideoStream( NULL ),
        mAudioStream( NULL ),
        mAvInputFrame( NULL ),
        mAvInterFrame( NULL ),
        mMinTime(minTime)
    {
    }
    ~VideoRecorder()
    {
    }
};

#endif // OZ_VIDEO_RECORDER_H

/*@}*/

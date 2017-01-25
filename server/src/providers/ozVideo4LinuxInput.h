/** @addtogroup Providers */
/*@{*/


#ifndef OZ_VIDEO_4_LINUX_INPUT_H
#define OZ_VIDEO_4_LINUX_INPUT_H

#include "../base/ozFeedProvider.h"
#include "../base/ozFeedFrame.h"
#include "../libgen/libgenThread.h"

#ifdef HAVE_LINUX_VIDEODEV2_H
#include <linux/videodev2.h>
#endif // HAVE_LINUX_VIDEODEV2_H
#include <sys/ioctl.h>

///
/// Class used for accessing Video4Linux2 streams via /dev/videoX devices. Accesses files directly using
/// v4l2 functions rather than ffmpeg libavcodec libaries. Can supply video from more than one channel per
/// video device, consumers should filter out the frames they do not want.
///
class Video4LinuxInput : public VideoProvider, public Thread
{
CLASSID(Video4LinuxInput);

public:
    /// Bit mask for accessing multiple channels per device
    enum {
        CHANNEL_0 = 0x00000001,   
        CHANNEL_1 = 0x00000002,   
        CHANNEL_2 = 0x00000004,   
        CHANNEL_3 = 0x00000008,   
        CHANNEL_4 = 0x00000010,   
        CHANNEL_5 = 0x00000020,   
        CHANNEL_6 = 0x00000040,   
        CHANNEL_7 = 0x00000080,   
        CHANNEL_8 = 0x00000100,   
        CHANNEL_9 = 0x00000200,   
        CHANNEL_10 = 0x00000400,   
        CHANNEL_11 = 0x00000800,   
        CHANNEL_12 = 0x00001000,   
        CHANNEL_13 = 0x00002000,   
        CHANNEL_14 = 0x00004000,   
        CHANNEL_15 = 0x00008000,   
    };

private:
    /// Video4Linux2 mapped buffer structure
    struct V4L2MappedBuffer
    {
        void    *start;
        size_t  length;
    };

    /// Video4Linux2 data structure
    struct V4L2Data
    {
        v4l2_cropcap        cropcap;
        v4l2_crop           crop;
        v4l2_format         fmt;
        v4l2_requestbuffers reqbufs;
        V4L2MappedBuffer    *buffers;
        v4l2_buffer         *bufptr;
    };

    typedef std::vector<int>    ChannelList;

private:
    std::string     mDevice;        ///< Source device
    int             mStandard;      ///< Requested video standard (PAL/NTSC etc)
    int             mPalette;       ///< Requested video palette, applies to all channels
    int             mWidth;         ///< Requested video width, applies to all channels
    int             mHeight;        ///< Requested video height, applies to all channels
    FrameRate       mFrameRate;     ///< Requested frame rate
    PixelFormat     mPixelFormat;   ///< FFmpeg equivalent image format
    uint32_t        mChannelMask;   ///< Bit mask indicating what channels are to be used
    ChannelList     mChannels;      ///< List of channels used
    int             mCurrentChannel;///< The current channel being decoded

private:
    // Utility function for doing ioctl calls that may require retries
    static int vidioctl( int fd, int request, void *arg )
    {
        int result = -1;
        do
        {
            result = ioctl( fd, request, arg );
        	//Debug( 3, "Result = %d, errno = %d", result, errno );
        } while ( result == -1 && errno == EINTR );
        return( result );
    }
    /// Convert a V4L2 palette into the ffmpeg equivalent
    static PixelFormat getPixelFormatFromV4lPalette( int palette );
    /// Comparator function used by channel comparators below to filter frames for specific channel
    static bool channelFrames( const FramePtr &, const FeedConsumer *, int channel );

public:
    /// Channel comparators. Used by consumer to limit video to one specific channel if prpvider is generating several.
    static bool channelFrames0( const FramePtr &, const FeedConsumer * );
    static bool channelFrames1( const FramePtr &, const FeedConsumer * );
    static bool channelFrames2( const FramePtr &, const FeedConsumer * );
    static bool channelFrames3( const FramePtr &, const FeedConsumer * );
    static bool channelFrames4( const FramePtr &, const FeedConsumer * );
    static bool channelFrames5( const FramePtr &, const FeedConsumer * );
    static bool channelFrames6( const FramePtr &, const FeedConsumer * );
    static bool channelFrames7( const FramePtr &, const FeedConsumer * );
    static bool channelFrames8( const FramePtr &, const FeedConsumer * );
    static bool channelFrames9( const FramePtr &, const FeedConsumer * );
    static bool channelFrames10( const FramePtr &, const FeedConsumer * );
    static bool channelFrames11( const FramePtr &, const FeedConsumer * );
    static bool channelFrames12( const FramePtr &, const FeedConsumer * );
    static bool channelFrames13( const FramePtr &, const FeedConsumer * );
    static bool channelFrames14( const FramePtr &, const FeedConsumer * );
    static bool channelFrames15( const FramePtr &, const FeedConsumer * );
         
public:
    Video4LinuxInput( const std::string &name, const std::string &device, int standard, int palette, int width, int height, uint32_t channelMask=CHANNEL_0 );
    ~Video4LinuxInput();

    uint16_t width() const
    {
        return( mWidth );
    }
    uint16_t height() const
    {
        return( mHeight );
    }
    PixelFormat pixelFormat() const
    {
        return( mPixelFormat );
    }
    FrameRate frameRate() const
    {
        return( mFrameRate );
    }

    int currentChannel() const
    {
        return( mCurrentChannel );
    }

protected:
    int run();
};

#endif // OZ_VIDEO_4_LINUX_INPUT_H


/*@}*/

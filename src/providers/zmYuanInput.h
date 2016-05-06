#ifndef ZM_YUAN_INPUT_H
#define ZM_YUAN_INPUT_H

#include "../zmFeedProvider.h"
#include "../zmFeedFrame.h"
#include "../libgen/libgenThread.h"

#ifdef HAVE_LINUX_VIDEODEV2_H
#include <linux/videodev2.h>
#endif // HAVE_LINUX_VIDEODEV2_H
#include <sys/ioctl.h>

#define V4L2_CID_HV_SCALE_FACTOR        (V4L2_CID_BASE + 204)
#define V4L2_CID_INPUT_SELECT           (V4L2_CID_BASE + 210)

#define V4L2_CID_X_RES_DETECTED         (V4L2_CID_BASE + 294)
#define V4L2_CID_Y_RES_DETECTED         (V4L2_CID_BASE + 295)
#define V4L2_CID_FPS_DETECTED           (V4L2_CID_BASE + 296)

#define V4L2_CID_MPEG_VIDEO_QUALITY     (V4L2_CID_MPEG_BASE + 250)
#define V4L2_CID_MPEG_VIDEO_FRAMERATE   (V4L2_CID_MPEG_BASE + 251)
#define V4L2_CID_MPEG_VIDEO_QPSTEP      (V4L2_CID_MPEG_BASE + 252)

typedef struct 
{
    uint32_t nFrameBufferSize : 24; // FRAME BUFFER SIZE (H.264 BITSTREAM LENGTH) = 0x00000000 ~ 0x00FFFFFFFF
    uint32_t nFrameType       :  1; // 0 = I FRAME / 1 = P FRAME
    uint32_t nMotionStatus    :  1; // 0 = STATIC / 1 = MOTION
    uint32_t nLockStatus      :  1; // 0 = UNLOCK / 1 = LOCK
    uint32_t nReserved        :  5;

} YuanH264Header;

///
/// Class used for accessing Video4Linux2 streams via /dev/videoX devices. Accesses files directly using
/// v4l2 functions rather than ffmpeg libavcodec libaries. Can supply video from more than one channel per
/// video device, consumers should filter out the frames they do not want.
///
class YuanInput : public VideoProvider, public Thread
{
CLASSID(YuanInput);

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
    bool            mH264Input;     ///< Whether the device is generating actual video frames, or H.264 frames

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

public:
    YuanInput( const std::string &name, const std::string &device, int standard, int palette, int width, int height, bool h264Input );
    ~YuanInput();

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

    bool h264Input() const
    {
        return( mH264Input );
    }

protected:
    int run();
};

#endif // ZM_YUAN_INPUT_H

#include "../zm.h"
#include "zmVideo4LinuxInput.h"

#include "../zmFfmpeg.h"
#include "../libgen/libgenDebug.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

PixelFormat Video4LinuxInput::getPixelFormatFromV4lPalette( int palette )
{
    PixelFormat pixFormat = PIX_FMT_NONE;
    switch( palette )
    {
#ifdef V4L2_PIX_FMT_RGB444
        case V4L2_PIX_FMT_RGB444 :
            pixFormat = PIX_FMT_RGB32;
            break;
#endif // V4L2_PIX_FMT_RGB444
        case V4L2_PIX_FMT_RGB555 :
            pixFormat = PIX_FMT_RGB555;
            break;
        case V4L2_PIX_FMT_RGB565 :
            pixFormat = PIX_FMT_RGB565;
            break;
        case V4L2_PIX_FMT_BGR24 :
            pixFormat = PIX_FMT_BGR24;
            break;
        case V4L2_PIX_FMT_RGB24 :
            pixFormat = PIX_FMT_RGB24;
            break;
        case V4L2_PIX_FMT_BGR32 :
            pixFormat = PIX_FMT_BGR32;
            break;
        case V4L2_PIX_FMT_RGB32 :
            pixFormat = PIX_FMT_RGB32;
            break;
        case V4L2_PIX_FMT_GREY :
            pixFormat = PIX_FMT_GRAY8;
            break;
        case V4L2_PIX_FMT_YUYV :
            pixFormat = PIX_FMT_YUYV422;
            break;
        case V4L2_PIX_FMT_UYVY :
            pixFormat = PIX_FMT_UYVY422;
            break;
        case V4L2_PIX_FMT_YUV422P :
            pixFormat = PIX_FMT_YUV422P;
            break;
        case V4L2_PIX_FMT_YUV411P :
            pixFormat = PIX_FMT_YUV411P;
            break;
#ifdef V4L2_PIX_FMT_YUV444
        case V4L2_PIX_FMT_YUV444 :
            pixFormat = PIX_FMT_YUV444P;
            break;
#endif // V4L2_PIX_FMT_YUV444
        case V4L2_PIX_FMT_YUV410 :
            pixFormat = PIX_FMT_YUV410P;
            break;
        //case V4L2_PIX_FMT_YVU410 :
            //pixFormat = PIX_FMT_YVU410P;
            //break;
        case V4L2_PIX_FMT_YUV420 :
        case V4L2_PIX_FMT_YVU420 :
            pixFormat = PIX_FMT_YUV420P;
            break;
        //case V4L2_PIX_FMT_YVU420 :
            //pixFormat = PIX_FMT_YVU420P;
            //break;
        case V4L2_PIX_FMT_JPEG :
            pixFormat = PIX_FMT_YUVJ444P;
            break;
        // These don't seem to have ffmpeg equivalents
        // See if you can match any of the ones in the default clause below!?
        case V4L2_PIX_FMT_VYUY :
        case V4L2_PIX_FMT_RGB332 :
        case V4L2_PIX_FMT_RGB555X :
        case V4L2_PIX_FMT_RGB565X :
        //case V4L2_PIX_FMT_Y16 :
        //case V4L2_PIX_FMT_PAL8 :
        case V4L2_PIX_FMT_YVU410 :
        case V4L2_PIX_FMT_Y41P :
        //case V4L2_PIX_FMT_YUV555 :
        //case V4L2_PIX_FMT_YUV565 :
        //case V4L2_PIX_FMT_YUV32 :
        case V4L2_PIX_FMT_NV12 :
        case V4L2_PIX_FMT_NV21 :
        case V4L2_PIX_FMT_YYUV :
        case V4L2_PIX_FMT_HI240 :
        case V4L2_PIX_FMT_HM12 :
        //case V4L2_PIX_FMT_SBGGR8 :
        //case V4L2_PIX_FMT_SGBRG8 :
        //case V4L2_PIX_FMT_SBGGR16 :
        case V4L2_PIX_FMT_MJPEG :
        case V4L2_PIX_FMT_DV :
        case V4L2_PIX_FMT_MPEG :
        case V4L2_PIX_FMT_WNVA :
        case V4L2_PIX_FMT_SN9C10X :
        case V4L2_PIX_FMT_PWC1 :
        case V4L2_PIX_FMT_PWC2 :
        case V4L2_PIX_FMT_ET61X251 :
        //case V4L2_PIX_FMT_SPCA501 :
        //case V4L2_PIX_FMT_SPCA505 :
        //case V4L2_PIX_FMT_SPCA508 :
        //case V4L2_PIX_FMT_SPCA561 :
        //case V4L2_PIX_FMT_PAC207 :
        //case V4L2_PIX_FMT_PJPG :
        //case V4L2_PIX_FMT_YVYU :
        default :
        {
            Fatal( "Can't find pixel format for palette %d", palette );
            break;
            // These are all spare and may match some of the above
            pixFormat = PIX_FMT_YUVJ420P;
            pixFormat = PIX_FMT_YUVJ422P;
            pixFormat = PIX_FMT_XVMC_MPEG2_MC;
            pixFormat = PIX_FMT_XVMC_MPEG2_IDCT;
            //pixFormat = PIX_FMT_UYVY422;
            pixFormat = PIX_FMT_UYYVYY411;
            pixFormat = PIX_FMT_BGR565;
            pixFormat = PIX_FMT_BGR555;
            pixFormat = PIX_FMT_BGR8;
            pixFormat = PIX_FMT_BGR4;
            pixFormat = PIX_FMT_BGR4_BYTE;
            pixFormat = PIX_FMT_RGB8;
            pixFormat = PIX_FMT_RGB4;
            pixFormat = PIX_FMT_RGB4_BYTE;
            pixFormat = PIX_FMT_NV12;
            pixFormat = PIX_FMT_NV21;
            pixFormat = PIX_FMT_RGB32_1;
            pixFormat = PIX_FMT_BGR32_1;
            pixFormat = PIX_FMT_GRAY16BE;
            pixFormat = PIX_FMT_GRAY16LE;
            pixFormat = PIX_FMT_YUV440P;
            pixFormat = PIX_FMT_YUVJ440P;
            pixFormat = PIX_FMT_YUVA420P;
            //pixFormat = PIX_FMT_VDPAU_H264;
            //pixFormat = PIX_FMT_VDPAU_MPEG1;
            //pixFormat = PIX_FMT_VDPAU_MPEG2;
        }
    }
    return( pixFormat );
}

Video4LinuxInput::Video4LinuxInput( const std::string &name, const std::string &device, int standard, int palette, int width, int height, uint32_t channelMask ) :
    VideoProvider( cClass(), name ),
    Thread( identity() ),
    mDevice( device ),
    mStandard( standard ),
    mPalette( palette ),
    mWidth( width ),
    mHeight( height ),
    mPixelFormat( getPixelFormatFromV4lPalette( mPalette ) ),
    mChannelMask( channelMask )
{
    uint32_t channelFlag = 1;
    for ( int i = 0; i < 16; i++ )
    {
        if ( mChannelMask & (channelFlag<<i) )
            mChannels.push_back( i );
    }
    mCurrentChannel = mChannels[0];
}

Video4LinuxInput::~Video4LinuxInput()
{
}

int Video4LinuxInput::run()
{
    struct stat st; 
    if ( stat( mDevice.c_str(), &st ) < 0 )
        Fatal( "Failed to stat video device %s: %s", mDevice.c_str(), strerror(errno) );

    if ( !S_ISCHR(st.st_mode) )
        Fatal( "File %s is not device file: %s", mDevice.c_str(), strerror(errno) );

    int vidFd = -1;
    Debug( 3, "Opening video device %s", mDevice.c_str() );
    //if ( (vidFd = open( device.c_str(), O_RDWR|O_NONBLOCK, 0 )) < 0 )
    if ( (vidFd = open( mDevice.c_str(), O_RDWR, 0 )) < 0 )
        Fatal( "Failed to open video device %s: %s", mDevice.c_str(), strerror(errno) );

    Debug( 3, "Querying video formats" );
    struct v4l2_fmtdesc format;
    memset( &format, 0, sizeof(format) );
    format.index = 0;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while( true )
    {
        if ( vidioctl( vidFd, VIDIOC_ENUM_FMT, &format ) < 0 )
        {
            if ( errno == EINVAL )
                break;
            else
                Fatal( "Failed to get video format: %s", strerror(errno) );
        }
        Debug( 3, "  %d - flags:%08x, description: '%s', pixel format %08x", format.index, format.flags, format.description, format.pixelformat );

        Debug( 3, "Querying frame sizes" );
        struct v4l2_frmsizeenum framesizes;
        memset( &framesizes, 0, sizeof(framesizes) );
        framesizes.index = 0;
        framesizes.pixel_format = format.pixelformat;

#if 0
        while( true )
        {
            if ( vidioctl( vidFd, VIDIOC_ENUM_FRAMESIZES, &framesizes ) < 0 )
            {
                if ( errno == EINVAL )
                    break;
                else
                    Fatal( "Failed to get frame sizes: %s", strerror(errno) );
            }
            switch ( framesizes.type )
            {
                case V4L2_FRMSIZE_TYPE_DISCRETE :
                    Debug( 3, "    %d - width:%d, height: %d", framesizes.index, framesizes.discrete.width, framesizes.discrete.height );
                    break;
                case V4L2_FRMSIZE_TYPE_CONTINUOUS :
                case V4L2_FRMSIZE_TYPE_STEPWISE :
                    Debug( 3, "    %d - type: %d, "
                        "min_width:%d, max_width: %d, step_width: %d"
                        "min_height:%d, max_height: %d, step_height: %d",
                        framesizes.index, framesizes.type,
                        framesizes.stepwise.min_width, framesizes.stepwise.max_width, framesizes.stepwise.step_width,
                        framesizes.stepwise.min_height, framesizes.stepwise.max_height, framesizes.stepwise.step_height
                    );
                    break;
                default :
                    Error( "Unexpected framesizes type %d", framesizes.type );
            }
            framesizes.index++;
        }
#endif
        format.index++;
    }

    Debug( 3, "Setting up video format" );
    V4L2Data v4l2Data;
    memset( &v4l2Data.fmt, 0, sizeof(v4l2Data.fmt) );
    v4l2Data.fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if ( vidioctl( vidFd, VIDIOC_G_FMT, &v4l2Data.fmt ) < 0 )
        Fatal( "Failed to get video format: %s", strerror(errno) );

    Debug( 4, "v4l2Data.fmt.type = %08x", v4l2Data.fmt.type );
    Debug( 4, "v4l2Data.fmt.fmt.pix.width = %d", v4l2Data.fmt.fmt.pix.width );
    Debug( 4, "v4l2Data.fmt.fmt.pix.height = %d", v4l2Data.fmt.fmt.pix.height );
    Debug( 4, "v4l2Data.fmt.fmt.pix.pixelformat = %08x", v4l2Data.fmt.fmt.pix.pixelformat );
    Debug( 4, "v4l2Data.fmt.fmt.pix.field = %08x", v4l2Data.fmt.fmt.pix.field );
    Debug( 4, "v4l2Data.fmt.fmt.pix.bytesperline = %d", v4l2Data.fmt.fmt.pix.bytesperline );
    Debug( 4, "v4l2Data.fmt.fmt.pix.sizeimage = %d", v4l2Data.fmt.fmt.pix.sizeimage );
    Debug( 4, "v4l2Data.fmt.fmt.pix.colorspace = %08x", v4l2Data.fmt.fmt.pix.colorspace );
    Debug( 4, "v4l2Data.fmt.fmt.pix.priv = %08x", v4l2Data.fmt.fmt.pix.priv );

    v4l2Data.fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2Data.fmt.fmt.pix.width = mWidth; 
    v4l2Data.fmt.fmt.pix.height = mHeight;
    v4l2Data.fmt.fmt.pix.pixelformat = mPalette;
 
#if 1
    //config.v4l2_capture_fields = V4L2_FIELD_INTERLACED;
    config.v4l2_capture_fields = V4L2_FIELD_TOP;
    if ( config.v4l2_capture_fields )
    {
        v4l2Data.fmt.fmt.pix.field = (v4l2_field)config.v4l2_capture_fields;
        if ( vidioctl( vidFd, VIDIOC_S_FMT, &v4l2Data.fmt ) < 0 )
        {
            Warning( "Failed to set V4L2 field to %d, falling back to auto", config.v4l2_capture_fields );
            v4l2Data.fmt.fmt.pix.field = V4L2_FIELD_ANY;
            if ( vidioctl( vidFd, VIDIOC_S_FMT, &v4l2Data.fmt ) < 0 )
                Fatal( "Failed to set video format: %s", strerror(errno) );
        }
    }
#endif
    if ( vidioctl( vidFd, VIDIOC_S_FMT, &v4l2Data.fmt ) < 0 )
        Fatal( "Failed to set video format: %s", strerror(errno) );

    if ( v4l2Data.fmt.fmt.pix.width != mWidth || v4l2Data.fmt.fmt.pix.height != mHeight )
    {
        Warning( "Resetting dimensions from %dx%d to %dx%d", mWidth, mHeight, v4l2Data.fmt.fmt.pix.width, v4l2Data.fmt.fmt.pix.height );
        mWidth = v4l2Data.fmt.fmt.pix.width;
        mHeight = v4l2Data.fmt.fmt.pix.height;
    }

    /* Note VIDIOC_S_FMT may change width and height. */
    Debug( 4, "v4l2Data.fmt.type = %08x", v4l2Data.fmt.type );
    Debug( 4, "v4l2Data.fmt.fmt.pix.width = %d", v4l2Data.fmt.fmt.pix.width );
    Debug( 4, "v4l2Data.fmt.fmt.pix.height = %d", v4l2Data.fmt.fmt.pix.height );
    Debug( 4, "v4l2Data.fmt.fmt.pix.pixelformat = %08x", v4l2Data.fmt.fmt.pix.pixelformat );
    Debug( 4, "v4l2Data.fmt.fmt.pix.field = %08x", v4l2Data.fmt.fmt.pix.field );
    Debug( 4, "v4l2Data.fmt.fmt.pix.bytesperline = %d", v4l2Data.fmt.fmt.pix.bytesperline );
    Debug( 4, "v4l2Data.fmt.fmt.pix.sizeimage = %d", v4l2Data.fmt.fmt.pix.sizeimage );
    Debug( 4, "v4l2Data.fmt.fmt.pix.colorspace = %08x", v4l2Data.fmt.fmt.pix.colorspace );
    Debug( 4, "v4l2Data.fmt.fmt.pix.priv = %08x", v4l2Data.fmt.fmt.pix.priv );
 
    /* Buggy driver paranoia. */
    unsigned int min;
    min = v4l2Data.fmt.fmt.pix.width * 2;
    if (v4l2Data.fmt.fmt.pix.bytesperline < min)
        v4l2Data.fmt.fmt.pix.bytesperline = min;
    min = v4l2Data.fmt.fmt.pix.bytesperline * v4l2Data.fmt.fmt.pix.height;
    if (v4l2Data.fmt.fmt.pix.sizeimage < min)
        v4l2Data.fmt.fmt.pix.sizeimage = min;

    struct v4l2_capability vid_cap;

    Debug( 3, "Checking video device capabilities" );
    if ( vidioctl( vidFd, VIDIOC_QUERYCAP, &vid_cap ) < 0 )
        Fatal( "Failed to query video device: %s", strerror(errno) );

    if ( !(vid_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) )
        Fatal( "Video device is not video capture device" );

    if ( !(vid_cap.capabilities & V4L2_CAP_STREAMING) )
        Fatal( "Video device does not support streaming i/o" );

    Debug( 3, "Setting up request buffers" );
    memset( &v4l2Data.reqbufs, 0, sizeof(v4l2Data.reqbufs) );
    if ( mChannels.size() > 1 )
        if ( config.v4l_multi_buffer )
            v4l2Data.reqbufs.count = 2*mChannels.size();
        else
            v4l2Data.reqbufs.count = 1;
    else
        v4l2Data.reqbufs.count = 8;
    v4l2Data.reqbufs.type = v4l2Data.fmt.type;
    v4l2Data.reqbufs.memory = V4L2_MEMORY_MMAP;

    if ( vidioctl( vidFd, VIDIOC_REQBUFS, &v4l2Data.reqbufs ) < 0 )
    {
        if ( errno == EINVAL )
        {
            Fatal( "Unable to initialise memory mapping, unsupported in device" );
        }
        else
        {
            Fatal( "Unable to initialise memory mapping: %s", strerror(errno) );
        }
    }

    if ( v4l2Data.reqbufs.count < (config.v4l_multi_buffer?2:1) )
        Fatal( "Insufficient buffer memory %d on video device", v4l2Data.reqbufs.count );

    Debug( 3, "Setting up %d data buffers", v4l2Data.reqbufs.count );

    v4l2Data.buffers = new V4L2MappedBuffer[v4l2Data.reqbufs.count];
    for ( int i = 0; i < v4l2Data.reqbufs.count; i++ )
    {
        struct v4l2_buffer vid_buf;

        memset( &vid_buf, 0, sizeof(vid_buf) );

        vid_buf.type = v4l2Data.fmt.type;
        vid_buf.memory = v4l2Data.reqbufs.memory;
        vid_buf.index = i;

        if ( vidioctl( vidFd, VIDIOC_QUERYBUF, &vid_buf ) < 0 )
            Fatal( "Unable to query video buffer: %s", strerror(errno) );

        v4l2Data.buffers[i].length = vid_buf.length;
        v4l2Data.buffers[i].start = mmap( NULL, vid_buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, vidFd, vid_buf.m.offset );

        if ( v4l2Data.buffers[i].start == MAP_FAILED )
            Fatal( "Can't map video buffer %d (%d bytes) to memory: %s(%d)", i, vid_buf.length, strerror(errno), errno );
    }

    int channelIndex = 0;
    mCurrentChannel = mChannels[channelIndex];

    Debug( 3, "Configuring video source" );
    if ( vidioctl( vidFd, VIDIOC_S_INPUT, &mCurrentChannel ) < 0 )
        Fatal( "Failed to set camera source %d: %s", mCurrentChannel, strerror(errno) );

    struct v4l2_input input;
    memset( &input, 0, sizeof(input) );
    if ( vidioctl( vidFd, VIDIOC_ENUMINPUT, &input ) < 0 )
        Fatal( "Failed to enumerate input %d: %s", mCurrentChannel, strerror(errno) );

    if ( (input.std != V4L2_STD_UNKNOWN) && ((input.std & mStandard) == V4L2_STD_UNKNOWN) )
        Fatal( "Device does not support video standard %d", mStandard );

    v4l2_std_id stdId = mStandard;
    if ( (input.std != V4L2_STD_UNKNOWN) && vidioctl( vidFd, VIDIOC_S_STD, &stdId ) < 0 )   
        Fatal( "Failed to set video standard %d: %s", mStandard, strerror(errno) );

    Debug( 3, "Queueing buffers" );
    for ( int frame = 0; frame < v4l2Data.reqbufs.count; frame++ )
    {
        struct v4l2_buffer vid_buf;

        memset( &vid_buf, 0, sizeof(vid_buf) );

        vid_buf.type = v4l2Data.fmt.type;
        vid_buf.memory = v4l2Data.reqbufs.memory;
        vid_buf.index = frame;

        if ( vidioctl( vidFd, VIDIOC_QBUF, &vid_buf ) < 0 )
            Fatal( "Failed to queue buffer %d: %s", frame, strerror(errno) );
    }
    v4l2Data.bufptr = NULL;

    Debug( 3, "Starting video stream" );
    //enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    enum v4l2_buf_type type = (v4l2_buf_type)v4l2Data.fmt.type;
    if ( vidioctl( vidFd, VIDIOC_STREAMON, &type ) < 0 )
        Fatal( "Failed to start capture stream: %s", strerror(errno) );
    Debug( 3, "Started video stream" );

#if 0
    AVProbeData probeData;
    probeData.filename = mDevice.c_str();
    probeData.buf = new unsigned char[1024];
    probeData.buf_size = 1024;
    inputFormat = av_probe_input_format( &probeData, 0 );
    if ( inputFormat == NULL)
        Fatal( "Can't probe input format" );
#endif

    //int captureWidth = mWidth;
    //int captureHeight = mHeight;
    struct timeval now;
    while( !mStop )
    {
        int captureFrame = -1;

        struct v4l2_buffer vid_buf;
        memset( &vid_buf, 0, sizeof(vid_buf) );

        vid_buf.type = v4l2Data.fmt.type;
        //vid_buf.memory = V4L2_MEMORY_MMAP;
        vid_buf.memory = v4l2Data.reqbufs.memory;

        int capturesPerFrame = 1;
        if ( mChannels.size() > 1 )
            capturesPerFrame = config.captures_per_frame;
        Debug( 3, "Capturing %d frames", capturesPerFrame );
        while ( capturesPerFrame )
        {
            if ( vidioctl( vidFd, VIDIOC_DQBUF, &vid_buf ) < 0 )
            {
                if ( errno == EIO )
                    Warning( "Capture failure, possible signal loss?: %s", strerror(errno) )
                else
                    Error( "Unable to capture frame %d: %s", vid_buf.index, strerror(errno) )
                return( -1 );
            }

            v4l2Data.bufptr = &vid_buf;
            captureFrame = v4l2Data.bufptr->index;
            if ( --capturesPerFrame )
            {
                if ( vidioctl( vidFd, VIDIOC_QBUF, &vid_buf ) < 0 )
                {
                    Error( "Unable to requeue buffer %d: %s", vid_buf.index, strerror(errno) );
                    return( -1 );
                }
            }
        }

        Debug( 3, "Captured frame %d/%d from channel %d", captureFrame, v4l2Data.bufptr->sequence, mCurrentChannel );

        //captureWidth = v4l2Data.fmt.fmt.pix.width;
        //captureHeight = v4l2Data.fmt.fmt.pix.height;
        gettimeofday( &now, NULL );

        uint64_t timestamp = (((uint64_t)now.tv_sec)*1000000LL)+now.tv_usec;
        //Debug( 3, "%d: TS: %lld, TS1: %lld, TS2: %lld, TS3: %.3f", time( 0 ), timestamp, packet.pts, ((1000000LL*packet.pts*videoStream->time_base.num)/videoStream->time_base.den), (((double)packet.pts*videoStream->time_base.num)/videoStream->time_base.den) );
        //Info( "%ld:TS: %lld, TS1: %lld, TS2: %lld, TS3: %.3f", time( 0 ), timestamp, packet.pts, ((1000000LL*packet.pts*videoStream->time_base.num)/videoStream->time_base.den), (((double)packet.pts*videoStream->time_base.num)/videoStream->time_base.den) );

        VideoFrame *videoFrame = new VideoFrame( this, mFrameCount++, timestamp, (uint8_t *)v4l2Data.buffers[v4l2Data.bufptr->index].start, v4l2Data.buffers[v4l2Data.bufptr->index].length );
        distributeFrame( FramePtr( videoFrame ) );

        Debug( 2, "Post-capturing" );
        // Requeue the buffer unless we need to switch or are a duplicate camera on a channel
        if ( mChannels.size() > 1 )
        {
            int nextChannel = (++channelIndex)%mChannels.size();
            Debug( 3, "Switching video source to %d", mChannels[nextChannel] );
            if ( vidioctl( vidFd, VIDIOC_S_INPUT, &mChannels[nextChannel] ) < 0 )
            {
                Error( "Failed to set camera source %d: %s", mChannels[nextChannel], strerror(errno) );
                return( -1 );
            }
            mCurrentChannel = nextChannel;
        }
        Debug( 3, "Requeueing buffer %d", v4l2Data.bufptr->index );
        if ( vidioctl( vidFd, VIDIOC_QBUF, v4l2Data.bufptr ) < 0 )
        {
            Error( "Unable to requeue buffer %d: %s", v4l2Data.bufptr->index, strerror(errno) )
            return( -1 );
        }
        usleep( INTERFRAME_TIMEOUT );
    }
    cleanup();

    Debug( 3, "Terminating video stream" );
    if ( vidioctl( vidFd, VIDIOC_STREAMOFF, & v4l2Data.fmt.type ) < 0 )
        Error( "Failed to stop capture stream: %s", strerror(errno) );

    Debug( 3, "Unmapping video buffers" );
    for ( int i = 0; i < v4l2Data.reqbufs.count; i++ )
        if ( munmap( v4l2Data.buffers[i].start, v4l2Data.buffers[i].length ) < 0 )
            Error( "Failed to munmap buffer %d: %s", i, strerror(errno) );

    close( vidFd );

    return( !ended() );
}

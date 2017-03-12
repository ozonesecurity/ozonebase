#include "oz.h"
#include "ozFeedFrame.h"

#include "ozFeedProvider.h"

/**
* @brief 
*
* @param provider
* @param id
* @param timestamp
* @param buffer
*/
VideoFrame::VideoFrame( VideoProvider *provider, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer ) :
    FeedFrame( provider, FRAME_TYPE_VIDEO, id, timestamp, buffer ),
    mVideoProvider( provider ),
    mPixelFormat( provider->pixelFormat() ),
    mWidth( provider->width() ),
    mHeight( provider->height() )
{
}

/**
* @brief 
*
* @param provider
* @param id
* @param timestamp
* @param buffer
* @param size
*/
VideoFrame::VideoFrame( VideoProvider *provider, uint64_t id, uint64_t timestamp, const uint8_t *buffer, size_t size ) :
    FeedFrame( provider, FRAME_TYPE_VIDEO, id, timestamp, buffer, size ),
    mVideoProvider( provider ),
    mPixelFormat( provider->pixelFormat() ),
    mWidth( provider->width() ),
    mHeight( provider->height() )
{
}

/**
* @brief 
*
* @param provider
* @param parent
* @param id
* @param timestamp
* @param buffer
*/
VideoFrame::VideoFrame( VideoProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer ) :
    FeedFrame( provider, parent, FRAME_TYPE_VIDEO, id, timestamp, buffer ),
    mVideoProvider( provider ),
    mPixelFormat( provider->pixelFormat() ),
    mWidth( provider->width() ),
    mHeight( provider->height() )
{
}

/**
* @brief 
*
* @param provider
* @param parent
* @param id
* @param timestamp
* @param buffer
* @param size
*/
VideoFrame::VideoFrame( VideoProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const uint8_t *buffer, size_t size ) :
    FeedFrame( provider, parent, FRAME_TYPE_VIDEO, id, timestamp, buffer, size ),
    mVideoProvider( provider ),
    mPixelFormat( provider->pixelFormat() ),
    mWidth( provider->width() ),
    mHeight( provider->height() )
{
}

/**
* @brief 
*
* @param provider
* @param parent
*/
VideoFrame::VideoFrame( VideoProvider *provider, const FramePtr &parent ) :
    FeedFrame( provider, parent ),
    mVideoProvider( provider ),
    mPixelFormat( provider->pixelFormat() ),
    mWidth( provider->width() ),
    mHeight( provider->height() )
{
}

/*
VideoFrame::VideoFrame( const VideoFrame &frame ) :
    FeedFrame( frame ),
    mVideoProvider( frame.mVideoProvider ),
    mPixelFormat( frame.mPixelFormat ),
    mWidth( frame.mWidth ),
    mHeight( frame.mHeight )
{
}
*/

/**
* @brief 
*
* @param provider
* @param id
* @param timestamp
* @param buffer
* @param samples
*/
AudioFrame::AudioFrame( AudioProvider *provider, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, uint16_t samples ) :
    FeedFrame( provider, FRAME_TYPE_AUDIO, id, timestamp, buffer ),
    mAudioProvider( provider ),
    mSampleFormat( provider->sampleFormat() ),
    mSampleRate( provider->sampleRate() ),
    mChannels( provider->channels() ),
    mSamples( samples )
{
}

/**
* @brief 
*
* @param provider
* @param parent
* @param id
* @param timestamp
* @param buffer
* @param samples
*/
AudioFrame::AudioFrame( AudioProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer, uint16_t samples ) :
    FeedFrame( provider, parent, FRAME_TYPE_AUDIO, id, timestamp, buffer ),
    mAudioProvider( provider ),
    mSampleFormat( provider->sampleFormat() ),
    mSampleRate( provider->sampleRate() ),
    mChannels( provider->channels() ),
    mSamples( samples )
{
}

/**
* @brief 
*
* @param provider
* @param parent
*/
AudioFrame::AudioFrame( AudioProvider *provider, const FramePtr &parent ) :
    FeedFrame( provider, parent ),
    mAudioProvider( provider ),
    mSampleFormat( provider->sampleFormat() ),
    mSampleRate( provider->sampleRate() ),
    mChannels( provider->channels() ),
    mSamples( provider->samples() )
{
}

/*
AudioFrame::AudioFrame( const AudioFrame &frame ) :
    FeedFrame( frame ),
    mAudioProvider( frame.mAudioProvider ),
    mSampleFormat( frame.mSampleFormat ),
    mSampleRate( frame.mSampleRate ),
    mChannels( frame.mChannels ),
    mSamples( frame.mSamples )
{
}
*/

/**
* @brief 
*
* @param provider
* @param id
* @param timestamp
*/
DataFrame::DataFrame( FeedProvider *provider, uint64_t id, uint64_t timestamp ) :
    FeedFrame( provider, FRAME_TYPE_DATA, id, timestamp )
{
}

/**
* @brief 
*
* @param provider
* @param id
* @param timestamp
* @param buffer
*/
DataFrame::DataFrame( FeedProvider *provider, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer ) :
    FeedFrame( provider, FRAME_TYPE_DATA, id, timestamp, buffer )
{
}

/**
* @brief 
*
* @param provider
* @param id
* @param timestamp
* @param buffer
* @param size
*/
DataFrame::DataFrame( FeedProvider *provider, uint64_t id, uint64_t timestamp, const uint8_t *buffer, size_t size ) :
    FeedFrame( provider, FRAME_TYPE_DATA, id, timestamp, buffer, size )
{
}

/**
* @brief 
*
* @param provider
* @param parent
* @param id
* @param timestamp
*/
DataFrame::DataFrame( FeedProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp ) :
    FeedFrame( provider, parent, FRAME_TYPE_DATA, id, timestamp )
{
}

/**
* @brief 
*
* @param provider
* @param parent
* @param id
* @param timestamp
* @param buffer
*/
DataFrame::DataFrame( FeedProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer ) :
    FeedFrame( provider, parent, FRAME_TYPE_DATA, id, timestamp, buffer )
{
}

/**
* @brief 
*
* @param provider
* @param parent
* @param id
* @param timestamp
* @param buffer
* @param size
*/
DataFrame::DataFrame( FeedProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const uint8_t *buffer, size_t size ) :
    FeedFrame( provider, parent, FRAME_TYPE_DATA, id, timestamp, buffer, size )
{
}

/**
* @brief 
*
* @param provider
* @param parent
*/
DataFrame::DataFrame( FeedProvider *provider, const FramePtr &parent ) :
    FeedFrame( provider, parent )
{
}

/*
DataFrame::DataFrame( const DataFrame &frame ) :
    FeedFrame( frame )
{
}
*/

#if 0
DataFrame::DataFrame( DataProvider *provider, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer ) :
    FeedFrame( provider, FRAME_TYPE_DATA, id, timestamp, buffer ),
    mDataProvider( provider )
{
}

DataFrame::DataFrame( DataProvider *provider, uint64_t id, uint64_t timestamp, const uint8_t *buffer, size_t size ) :
    FeedFrame( provider, FRAME_TYPE_DATA, id, timestamp, buffer, size ),
    mDataProvider( provider )
{
}

DataFrame::DataFrame( DataProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const ByteBuffer &buffer ) :
    FeedFrame( provider, parent, FRAME_TYPE_DATA, id, timestamp, buffer ),
    mDataProvider( provider )
{
}

DataFrame::DataFrame( DataProvider *provider, const FramePtr &parent, uint64_t id, uint64_t timestamp, const uint8_t *buffer, size_t size ) :
    FeedFrame( provider, parent, FRAME_TYPE_DATA, id, timestamp, buffer, size ),
    mDataProvider( provider )
{
}

DataFrame::DataFrame( DataProvider *provider, const FramePtr &parent ) :
    FeedFrame( provider, parent ),
    mDataProvider( provider )
{
}

/*
DataFrame::DataFrame( const DataFrame &frame ) :
    FeedFrame( frame ),
    mDataProvider( frame.mDataProvider ),
{
}
*/
#endif

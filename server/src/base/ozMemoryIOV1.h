#ifndef OZ_MEMORY_IO_V1_H
#define OZ_MEMORY_IO_V1_H

#include "ozFfmpeg.h"

class Image;

class MemoryIOV1
{
protected:
    std::string     mLocation;
    int             mMemoryKey;
    bool            mOwner;
    int             mImageBufferCount;

    typedef enum
    {
        IDLE,
        PREALARM,
        ALARM,
        ALERT,
        TAPE
    } State;

    typedef struct
    {
        uint32_t size;         /* +0  */
        uint32_t last_write_index;   /* +4  */ 
        uint32_t last_read_index;    /* +8  */
        uint32_t state;        /* +12   */
        uint32_t last_event;       /* +16   */
        uint32_t action;         /* +20   */
        int32_t brightness;      /* +24   */
        int32_t hue;           /* +28   */
        int32_t colour;        /* +32   */
        int32_t contrast;        /* +36   */
        int32_t alarm_x;         /* +40   */
        int32_t alarm_y;         /* +44   */
        uint8_t valid;         /* +48   */
        uint8_t active;        /* +49   */
        uint8_t signal;        /* +50   */
        uint8_t format;        /* +51   */
        uint32_t imagesize;      /* +52   */
        uint32_t epadding1;      /* +56   */
        uint32_t epadding2;      /* +60   */
        /* 
        ** This keeps 32bit time_t and 64bit time_t identical and compatible as long as time is before 2038.
        ** Shared memory layout should be identical for both 32bit and 64bit and is multiples of 16.
        */  
        union {            /* +64  */
            time_t last_write_time;
            uint64_t extrapad1;
        };
        union {            /* +72   */
            time_t last_read_time;
            uint64_t extrapad2;
        };
        uint8_t control_state[256];  /* +80   */
    } SharedData;

    typedef enum { TRIGGER_CANCEL, TRIGGER_ON, TRIGGER_OFF } TriggerState;
    typedef struct
    {
        uint32_t size;
        uint32_t trigger_state;
        uint32_t trigger_score;
        uint32_t padding;
        char trigger_cause[32];
        char trigger_text[256];
        char trigger_showtext[256];
    } TriggerData;

    struct Snapshot
    {
        struct timeval  *timestamp;
        Image           *image;
        void            *padding;
    };

#if OZ_MEM_MAPPED
    int             mMapFd;
    char            mMemFile[PATH_MAX];
#else // OZ_MEM_MAPPED
    int             mShmId;
#endif // OZ_MEM_MAPPED
    int             mMemSize;
    unsigned char   *mMemPtr;

    SharedData      *mSharedData;
    TriggerData     *mTriggerData;

    Snapshot        *mImageBuffer;

protected:
    bool queryMemory( SharedData *sharedData );
    void attachMemory( int imageBufferCount, PixelFormat imageFormat, uint16_t imageWidth, uint16_t imageHeight );
    void attachMemory();
    void detachMemory();

public:
    MemoryIOV1( const std::string &location, int id, bool owner );
    ~MemoryIOV1();

    bool setTrigger( uint32_t score, const std::string &cause, const std::string &detail );
    bool clearTrigger();
};

#endif // OZ_MEMORY_IO_V1_H

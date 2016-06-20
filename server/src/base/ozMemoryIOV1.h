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
    int             mImageCount;

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
        int size;
        bool valid;
        bool active;
        bool signal;
        State state;
        int last_write_index;
        int last_read_index;
        time_t last_write_time;
        time_t last_read_time;
        int last_event;
        int action;
        int brightness;
        int hue;
        int colour;
        int contrast;
        int alarm_x;
        int alarm_y;
        char control_state[256];
    } SharedData;

    typedef enum { TRIGGER_CANCEL, TRIGGER_ON, TRIGGER_OFF } TriggerState;
    typedef struct
    {
        int size;
        TriggerState trigger_state;
        int trigger_score;
        char trigger_cause[32];
        char trigger_text[256];
        char trigger_showtext[256];
    } TriggerData;

    struct Snapshot
    {
        struct timeval  *timestamp;
        Image           *image;
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
    void attachMemory( int imageCount, AVPixelFormat imageFormat, uint16_t imageWidth, uint16_t imageHeight );
    void detachMemory();

public:
    MemoryIOV1( const std::string &location, int id, bool owner );
    ~MemoryIOV1();
};

#endif // OZ_MEMORY_IO_V1_H

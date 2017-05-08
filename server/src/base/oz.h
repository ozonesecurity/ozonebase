#ifndef OZ_H
#define OZ_H

extern "C"
{
#include "../libgen/libgenDebug.h"
}

#include "ozConfig.h"
#include <stdint.h>
#include <assert.h>
#ifdef __APPLE__
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#ifndef MAP_LOCKED
        #define MAP_LOCKED 0
#endif
#endif
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

extern "C"
{
#if !HAVE_DECL_ROUND
double round(double);
#endif
}

#endif // OZ_H

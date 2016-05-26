#ifndef ZM_H
#define ZM_H

extern "C"
{
#include "../libgen/libgenDebug.h"
}

#include "zmConfig.h"
#include <stdint.h>
#include <assert.h>

extern "C"
{
#if !HAVE_DECL_ROUND
double round(double);
#endif
}

#endif // ZM_H

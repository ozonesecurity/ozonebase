#ifndef LIBIMG_JPEG_H
#define LIBIMG_JPEG_H

#include "libimgJinclude.h"
#include "jpeglib.h"
#include "jerror.h"

#include <setjmp.h>

/* Stuff for overriden error handlers */
struct local_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct local_error_mgr *local_error_ptr;

void local_jpeg_error_exit( j_common_ptr cinfo );
void local_jpeg_emit_message( j_common_ptr cinfo, int msg_level );

// Prototypes for memory compress/decompression object */
void local_jpeg_mem_src(j_decompress_ptr cinfo, const JOCTET *inbuffer, int inbuffer_size );
void local_jpeg_mem_dest(j_compress_ptr cinfo, JOCTET *outbuffer, int *outbuffer_size );

#endif // LIBIMG_JPEG_H

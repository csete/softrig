#pragma once

#include <stdint.h>

#include "common/library_loader.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* opaque rtlsds reader structure */
struct _rtlsdr_reader;
typedef struct _rtlsdr_reader rtlsdr_reader_t;

/*
 * Create a new rtlsdr reader object.
 * rtldev is the rtlsdr device handle.
 * Returns a pointer to a newly created rtlsdr reader object.
 */
rtlsdr_reader_t * rtlsdr_reader_create(void * rtldev, lib_handle_t lib);

void    rtlsdr_reader_destroy(rtlsdr_reader_t * reader);

/*
 * Start the reader thread.
 * 
 * Returns 0 if the reader thread was started, otherwise the error code
 * returned by pthread_create().
 */
int     rtlsdr_reader_start(rtlsdr_reader_t * reader);

/*
 * Stop the rtlsdr reader thread.
 *
 * Returns 0  if the reader thread was started, -1 otherwise
 */
int     rtlsdr_reader_stop(rtlsdr_reader_t * reader);

uint32_t    rtlsdr_reader_get_num_bytes(rtlsdr_reader_t * reader);
uint32_t    rtlsdr_reader_read_bytes(rtlsdr_reader_t * reader, void * buffer,
                                     uint32_t bytes);

#ifdef __cplusplus
}
#endif

#pragma once

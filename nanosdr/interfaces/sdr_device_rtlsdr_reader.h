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

/**
 * Create a new rtlsdr reader object.
 * @param rtldev The rtlsdr device handle.
 * @returns Pointer to a newly created rtlsdr reader object.
 */
rtlsdr_reader_t * rtlsdr_reader_create(void * rtldev, lib_handle_t lib);

/**
 * Destroy previously created rtlsdr reader.
 * @param reader The rtlsdr reader handle.
 */
void    rtlsdr_reader_destroy(rtlsdr_reader_t * reader);

/**
 * Start the reader thread.
 * @param reader The rtlsdr reader handle.
 * @returns 0 if the reader thread was started, otherwise the error code
 *          returned by pthread_create().
 */
int     rtlsdr_reader_start(rtlsdr_reader_t * reader);

/**
 * Stop the rtlsdr reader thread.
 * @param reader The rtlsdr reader handle.
 * @retval  0 The reader thread was started.
 * @retval -1 An error occured trying to start the reader thread.
 */
int     rtlsdr_reader_stop(rtlsdr_reader_t * reader);

uint32_t    rtlsdr_reader_get_num_bytes(rtlsdr_reader_t * reader);
uint32_t    rtlsdr_reader_read_bytes(rtlsdr_reader_t * reader, void * buffer,
                                     uint32_t bytes);

#ifdef __cplusplus
}
#endif

#pragma once

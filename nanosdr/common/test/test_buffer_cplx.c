/*
 * Ring buffer test (complex_t API)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../datatypes.h"
#include "../ring_buffer_cplx.h"

static int failed = 0;
static int passed = 0;


static void test_int(const char *string, int var, int value)
{
    fprintf(stderr, "%s %d (exp: %d) ... ", string, var, value);

    if (var == value)
    {
        passed++;
        fprintf(stderr, "PASSED\n");
    }
    else
    {
        failed++;
        fprintf(stderr, "FAILED\n");
    }
}


int main(void)
{
    int retval = 0;
    int i;
    int buff_size;
    int write_size, read_size;
    int es = sizeof(complex_t);

    ring_buffer_t *rb = ring_buffer_cplx_create();

    /* random seed */
    srand(time(NULL));

    /* test 1 */
    fprintf(stderr, "\nTEST 1 - Standard read and write\n");
    buff_size = 10;
    ring_buffer_cplx_init(rb, buff_size);

    write_size = 7;
    read_size = 7;
    complex_t *wrbuf = (complex_t *) calloc(write_size, es);
    complex_t *rdbuf = (complex_t *) calloc(read_size, es);
    for (i = 0; i < write_size; i++)
    {
        wrbuf[i].re = 1.e-3 * (real_t) rand();
        wrbuf[i].im = 1.e-3 * (real_t) rand();
    }

    fprintf(stderr, "  Write 7 elements (%d bytes each)\n", es);
    ring_buffer_cplx_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, write_size * es);
    test_int("    Check rb->start:", rb->start, 0);

    fprintf(stderr, "  Read  7 elements (%d bytes each)\n", es);
    ring_buffer_cplx_read(rb, rdbuf, read_size);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 7 * es);
    test_int("    Compare R/W buf:", memcmp(wrbuf, rdbuf, 7 * es), 0);

    free(wrbuf);
    free(rdbuf);

    /* test 2 */
    fprintf(stderr, "\nTEST 2 - Write over the edge\n");
    wrbuf = (complex_t *) calloc(write_size, es);
    rdbuf = (complex_t *) calloc(read_size, es);
    for (i = 0; i < write_size; i++)
    {
        wrbuf[i].re = 1.e-3 * (real_t) rand();
        wrbuf[i].im = 1.e-3 * (real_t) rand();
    }

    fprintf(stderr, "  Write 7 elements (%d bytes each)\n", es);
    ring_buffer_cplx_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, write_size * es);
    test_int("    Check rb->start:", rb->start, 7 * es);

    fprintf(stderr, "  Read  7 elements (%d bytes each)\n", es);
    ring_buffer_cplx_read(rb, rdbuf, read_size);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 4 * es);
    test_int("    Compare R/W buf:", memcmp(wrbuf, rdbuf, 7 * es), 0);

    free(wrbuf);
    free(rdbuf);

    /* test 3 */
    write_size = 7;
    read_size = 10;

    fprintf(stderr, "\nTEST 3 - Overwrite existing data\n");
    wrbuf = (complex_t *) calloc(write_size, es);
    rdbuf = (complex_t *) calloc(read_size, es);
    complex_t *cmpbuf = (complex_t *) calloc(read_size, es);
    
    for (i = 0; i < write_size; i++)
    {
        wrbuf[i].re = 1.e-3 * (real_t) rand();
        wrbuf[i].im = 1.e-3 * (real_t) rand();
    }

    fprintf(stderr, "  Clear ring buffer\n");
    ring_buffer_cplx_clear(rb);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 0);

    fprintf(stderr, "  Write 7 elements (%d bytes each)\n", es);
    ring_buffer_cplx_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, write_size * es);
    test_int("    Check rb->start:", rb->start, 0);

    /* store the last 3 elements written as they will be the beginning
     * of the 10 bytes read back from the full buffer
     */
    memcpy(cmpbuf, &wrbuf[4], 3 * es);

    fprintf(stderr, "  Write 7 more elements (%d bytes each)\n", es);
    for (i = 0; i < write_size; i++)
    {
        wrbuf[i].re = 1.e-3 * (real_t) rand();
        wrbuf[i].im = 1.e-3 * (real_t) rand();
    }

    ring_buffer_cplx_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, 10 * es);
    test_int("    Check rb->start:", rb->start, 4 * es);

    /* store written bytes in compare buffer */
    memcpy(&cmpbuf[3], wrbuf, 7 * es);

    fprintf(stderr, "  Read 10 elements (%d bytes each)\n", es);
    ring_buffer_cplx_read(rb, rdbuf, read_size);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 4 * es);
    test_int("    Compare buffers:", memcmp(cmpbuf, rdbuf, 10 * es), 0);

    free(cmpbuf);
    free(wrbuf);
    free(rdbuf);

    /* test 4 -- write and read exactly rb->size amount of data */
    write_size = 8192;
    read_size = 8192;
    ring_buffer_cplx_resize(rb, 8192);

    fprintf(stderr, "\nTEST 4 - Write and read exactly buffer_size bytes\n");
    wrbuf = (complex_t *) calloc(write_size, es);
    rdbuf = (complex_t *) calloc(read_size, es);
    
    fprintf(stderr, "  Write data (%d elements)\n", write_size);
    for (i = 0; i < write_size; i++)
    {
        wrbuf[i].re = 1.e-3 * (real_t) rand();
        wrbuf[i].im = 1.e-3 * (real_t) rand();
    }
    ring_buffer_cplx_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, write_size * es);
    test_int("    Check rb->start:", rb->start, 0);

    fprintf(stderr, "  Read data (%d elements)\n", read_size);
    ring_buffer_cplx_read(rb, rdbuf, read_size);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 0);
    test_int("    Compare buffers:", memcmp(wrbuf, rdbuf, read_size * es), 0);

    free(wrbuf);
    free(rdbuf);

    fprintf(stderr, "\n\nFIXME: These tests should use API instead oif rb->count\n");

    fprintf(stderr, "\nTest summary:\n");
    fprintf(stderr, "    Passed: %d\n", passed);
    fprintf(stderr, "    Failed: %d\n\n", failed);

    /* clean up before exiting */
    ring_buffer_cplx_delete(rb);

    return retval;
}

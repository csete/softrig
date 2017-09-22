/*
 * Ring buffer test
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../ring_buffer.h"


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

    ring_buffer_t *rb = ring_buffer_create();

    /* random seed */
    srand(time(NULL));

    /* test 1 */
    fprintf(stderr, "\nTEST 1 - Standard read and write\n");
    buff_size = 10;
    ring_buffer_init(rb, buff_size);

    write_size = 7;
    read_size = 7;
    unsigned char *wrbuf = (unsigned char *) calloc(write_size, 1);
    unsigned char *rdbuf = (unsigned char *) calloc(read_size, 1);
    for (i = 0; i < write_size; i++)
        wrbuf[i] = (unsigned char)(rand() % 255);

    fprintf(stderr, "  Write 7 bytes\n");
    ring_buffer_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, write_size);
    test_int("    Check rb->start:", rb->start, 0);

    fprintf(stderr, "  Read  7 bytes\n");
    ring_buffer_read(rb, rdbuf, read_size);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 7);
    test_int("    Compare R/W buf:", memcmp(wrbuf, rdbuf, 7), 0);

    free(wrbuf);
    free(rdbuf);


    /* test 2 */
    fprintf(stderr, "\nTEST 2 - Write over the edge\n");
    wrbuf = (unsigned char *) calloc(write_size, 1);
    rdbuf = (unsigned char *) calloc(read_size, 1);
    for (i = 0; i < write_size; i++)
        wrbuf[i] = (unsigned char)(rand() % 255);

    fprintf(stderr, "  Write 7 bytes\n");
    ring_buffer_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, write_size);
    test_int("    Check rb->start:", rb->start, 7);

    fprintf(stderr, "  Read  7 bytes\n");
    ring_buffer_read(rb, rdbuf, read_size);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 4);
    test_int("    Compare R/W buf:", memcmp(wrbuf, rdbuf, 7), 0);

    free(wrbuf);
    free(rdbuf);

    /* test 3 */
    write_size = 7;
    read_size = 10;

    fprintf(stderr, "\nTEST 3 - Overwrite existing data\n");
    wrbuf = (unsigned char *) calloc(write_size, 1);
    rdbuf = (unsigned char *) calloc(read_size, 1);
    unsigned char *cmpbuf = (unsigned char *) calloc(read_size, 1);
    
    for (i = 0; i < write_size; i++)
        wrbuf[i] = (unsigned char)(rand() % 255);

    fprintf(stderr, "  Clear ring buffer\n");
    ring_buffer_clear(rb);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 0);

    fprintf(stderr, "  Write 7 bytes\n");
    ring_buffer_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, write_size);
    test_int("    Check rb->start:", rb->start, 0);

    /* store the last 3 bytes written as they will be the beginning
     * of the 10 bytes read back from the full buffer
     */
    memcpy(cmpbuf, &wrbuf[4], 3);

    fprintf(stderr, "  Write 7 more bytes\n");
    for (i = 0; i < write_size; i++)
        wrbuf[i] = (unsigned char)(rand() % 255);
    ring_buffer_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, 10);
    test_int("    Check rb->start:", rb->start, 4);

    /* store written bytes in compare buffer */
    memcpy(&cmpbuf[3], wrbuf, 7);

    fprintf(stderr, "  Read 10 bytes\n");
    ring_buffer_read(rb, rdbuf, read_size);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 4);
    test_int("    Compare buffers:", memcmp(cmpbuf, rdbuf, 10), 0);

    free(cmpbuf);
    free(wrbuf);
    free(rdbuf);

    /* test 4 -- write and read exactly rb->size amount of data */
    write_size = 8192;
    read_size = 8192;
    ring_buffer_resize(rb, 8192);

    fprintf(stderr, "\nTEST 4 - Write and read exactly buffer_size bytes\n");
    wrbuf = (unsigned char *) calloc(write_size, 1);
    rdbuf = (unsigned char *) calloc(read_size, 1);
    
    fprintf(stderr, "  Write data (%d bytes)\n", write_size);
    for (i = 0; i < write_size; i++)
        wrbuf[i] = (unsigned char)(rand() % 255);
    ring_buffer_write(rb, wrbuf, write_size);
    test_int("    Check rb->count:", rb->count, write_size);
    test_int("    Check rb->start:", rb->start, 0);

    fprintf(stderr, "  Read data (%d bytes)\n", read_size);
    ring_buffer_read(rb, rdbuf, read_size);
    test_int("    Check rb->count:", rb->count, 0);
    test_int("    Check rb->start:", rb->start, 0);
    test_int("    Compare buffers:", memcmp(wrbuf, rdbuf, 10), 0);

    fprintf(stderr, "\nTest summary:\n");
    fprintf(stderr, "    Passed: %d\n", passed);
    fprintf(stderr, "    Failed: %d\n\n", failed);

    /* clean up before exiting */
    ring_buffer_delete(rb);
    free(wrbuf);
    free(rdbuf);

    return retval;
}

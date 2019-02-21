/*
 * CTL queue.
 */
#pragma once
#include "sdr_ctl.h"
#include <stdint.h>

/* clang-format off */
// return codes
#define SDR_CTL_QUEUE_OK     0
#define SDR_CTL_QUEUE_ERROR -1
#define SDR_CTL_QUEUE_FULL  -2
#define SDR_CTL_QUEUE_EMPTY -3
/* clang-format on */

/*
 * A specialized version of a ring buffer used for CTLs, where a CTL is replaced
 * when a new CTL of the same type is added.
 *
 * NOTE: The replace CTL mechanism is not implemented.
 */
class SdrCtlQueue
{
  public:
    explicit SdrCtlQueue();
    virtual ~SdrCtlQueue();

    /*
     * (Re)initialize the queue.
     *
     * n is the number of CTLs the buffer can hold
     *
     * Returns:
     *   SDR_CTL_QUEUE_OK     The queue was successfully initialized.
     *   SDR_CTL_QUEUE_ERROR  The queue could not be initialized.
     */
    int init(unsigned int n);

    /** Clear the queue */
    void clear(void);

    /*
     * Add CTL to the queue.
     *
     * Returns:
     *   SDR_CTL_QUEUE_OK    The CTL was successfully added to the queue.
     *   SDR_CTL_QUEUE_ERROR If the CTL is 0.
     *   SDR_CTL_QUEUE_FULL  The CTL was not added because the buffer is
     *                       already full.
     */
    int add_ctl(const sdr_ctl_t *ctl);

    /*
     * Get next CTL from the queue.
     *
     * Returns:
     *   SDR_CTL_QUEUE_OK    A CTL was successfully retrieved from the queue.
     *   SDR_CTL_QUEUE_ERROR If the CTL is 0.
     *   SDR_CTL_QUEUE_EMPTY There are no CTLs in the queue.
     */
    int get_ctl(sdr_ctl_t *ctl);

    uint64_t get_overflows(void) const
    {
        return overflows;
    }

    unsigned int get_count(void) const
    {
        return num;
    }

  private:
    void free_memory(void);

  private:
    unsigned int head;  // index of the first CTL in the buffer
    unsigned int num;   // number of CTLs currently in buffer
    unsigned int size;  // number of CTLs the buffer can hold
    sdr_ctl_t *  buffer;

    uint64_t overwrites;  // number of times CTLs are replaced
    uint64_t overflows;   // number of times buffer was full when adding CTLs
};

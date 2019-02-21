/*
 * Various bit hacks by Sean Eron Anderson (public domain)
 * http://graphics.stanford.edu/~seander/bithacks.html
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* could also use (x && ((x & (~x + 1)) == x)) */
inline unsigned int is_power_of_two(unsigned int v)
{
    return (v && !(v & (v - 1)));
}

/* Round up to the next highest power of 2 */
inline unsigned int next_power_of_two(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    v += (v == 0); /* if v = 0 return 1 */

    return v;
}

#ifdef __cplusplus
}
#endif

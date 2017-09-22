/*
 * Signal strength meter
 */
#pragma once

#include "common/datatypes.h"

class SMeter
{
public:
    real_t      process(int num, complex_t * samples);
    real_t      get_signal_power(void) const { return rms_db; };

private:
    real_t      rms_db;
};

#ifndef __DEFAULT_FEEDER_H
#define __DEFAULT_FEEDER_H
#include "adec.h"
extern adec_feeder_t *get_default_adec_feeder(void);
extern adec_feeder_t *feeder_init(int dsp);
extern int feeder_release(void);
#endif

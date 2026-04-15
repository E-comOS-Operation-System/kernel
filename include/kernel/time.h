#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <stdint.h>

uint64_t timeGetCurrentMs(void);
void     timeTick(void);

#endif

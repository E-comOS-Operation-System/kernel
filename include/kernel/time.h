#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <stdint.h>

// Get the current time in milliseconds
uint64_t time_get_current_ms(void);

#endif // KERNEL_TIME_H
#include <kernel/time.h>

// Simple time counter (needs timer interrupt to update)
static volatile uint64_t system_ticks = 0;

uint64_t time_get_current_ms(void) {
    // Assume each tick is 1ms (needs timer configuration)
    return system_ticks;
}

// Timer interrupt handler (needs to be called from interrupt module)
void time_tick(void) {
    system_ticks++;
}
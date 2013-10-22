#include <stdint.h>
#include <stdbool.h>

/* pebble SDK doesn't treat unix time as it should; compensate for it */
static const uint32_t OFFSET_LEAP_SECONDS = 8;
static const uint32_t OFFSET_TIMEZONE = 0;

uint32_t unixtime(void);

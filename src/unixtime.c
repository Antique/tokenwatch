#include "unixtime.h"
#include "config.h"

#include <time.h>

uint32_t
unixtime(void)
{
    time_t localtime = time(NULL);

    /* compensate DST and timezone */
    localtime -= OFFSET_TIMEZONE;

#ifdef OFFSET_DST
    localtime -= 3600;
#endif

   return localtime;
}

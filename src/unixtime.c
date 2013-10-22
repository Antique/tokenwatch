#include "unixtime.h"

#include <time.h>

static bool
is_dst(void)
{
    /* TODO: */
    /* DST SUCKS!@!@!!! */
    /* DST in CET is from
     * last sunday in March, 00:03:00, to
     * last sunday in October, 00:02:00, but this check will slip 1 hour */

    return true;
    //return false;
}

uint32_t
unixtime(void)
{
    time_t localtime = time(NULL);

    /* compensate leap seconds */
    localtime += OFFSET_LEAP_SECONDS;

    /* compensate DST and timezone */
    localtime -= OFFSET_TIMEZONE;

    if (is_dst())
        localtime -= 3600;

   return localtime;
}

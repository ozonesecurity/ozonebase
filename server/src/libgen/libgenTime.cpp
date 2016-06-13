#include "libgenTime.h"

static char gmtDays[][4] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

static char gmtMons[][4] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

/// Internet date/time according to RFC1123
/**
* @brief 
*
* @return 
*/
std::string datetimeInet() 
{
    time_t now = time( NULL );
    struct tm gmtNow;
    gmtime_r( &now, &gmtNow );
    char timeString[32];
    snprintf( timeString, sizeof(timeString),
        "%s, %2d %s %4d %2d:%02d:%02d GMT", 
        gmtDays[gmtNow.tm_wday],
        gmtNow.tm_mday,
        gmtMons[gmtNow.tm_mon],
        1900+gmtNow.tm_year,
        gmtNow.tm_hour,
        gmtNow.tm_min,
        gmtNow.tm_sec
    );
    return( timeString );
}

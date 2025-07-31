
#include <string>
#include <stdio.h>
#include <sys/time.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>

static inline std::string
get_utc_string(const struct timeval &tv)
{
    struct tm broken_down_time;
    gmtime_r(&tv.tv_sec, &broken_down_time);

    // format the time according to the agent's ICD.
    // "YYYY-MM-DD HH:MM:SS" is 19 characters.
    // ".UUUUUU" is 7 characters. A buffer of 32 is safe.
    // Format the date and time part of the string, excluding microseconds.
    // Using the explicit format for maximum compatibility.
    // then, append the microseconds part.
    // NOTE the hardcode of 19 for the length of this format!
    char time_buffer[32];
    strftime(time_buffer, sizeof(time_buffer),
             "%Y-%m-%d %H:%M:%S", &broken_down_time);
    snprintf(time_buffer+19, sizeof(time_buffer)-19, ".%06ld",
             tv.tv_usec);
    return std::string(time_buffer);
}

int main()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("%s\n", get_utc_string(tv).c_str());
    return 0;
}

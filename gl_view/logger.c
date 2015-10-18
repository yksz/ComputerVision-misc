#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static LogLevel current_level = LogLevel_INFO;

void logger_write(LogLevel level, FILE* fp, const char* file, int line, const char* func, const char* fmt, ...)
{
    if (current_level > level) {
        return;
    }

    time_t timer = time(NULL);
    char time[20];
    tzset();
    strftime(time, sizeof(time), "%Y/%m/%d %H:%M:%S", localtime(&timer));

    switch (level) {
        case LogLevel_DEBUG:
            fprintf(fp, "%s DEBUG %s:%d:%s: ", time, file, line, func);
            break;
        case LogLevel_INFO:
            fprintf(fp, "%s INFO  %s:%d:%s: ", time, file, line, func);
            break;
        case LogLevel_WARN:
            fprintf(fp, "%s WARN  %s:%d:%s: ", time, file, line, func);
            break;
        case LogLevel_ERROR:
            fprintf(fp, "%s ERROR %s:%d:%s: ", time, file, line, func);
            break;
    }

    va_list list;
    va_start(list, fmt);
    vfprintf(fp, fmt, list);
    va_end(list);
}

void logger_setLevel(LogLevel new_level)
{
    current_level = new_level;
}

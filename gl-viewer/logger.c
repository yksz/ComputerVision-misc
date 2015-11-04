#include "logger.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static LogLevel currentLevel = LogLevel_INFO;

void logger_write(LogLevel level, FILE* fp, const char* file, int line, const char* func, const char* fmt, ...)
{
    if (currentLevel > level) {
        return;
    }

    time_t now = time(NULL);
    char timestr[20];
    strftime(timestr, sizeof(timestr), "%Y/%m/%d %H:%M:%S", localtime(&now));

    switch (level) {
        case LogLevel_DEBUG:
            fprintf(fp, "%s DEBUG %s:%d:%s: ", timestr, file, line, func);
            break;
        case LogLevel_INFO:
            fprintf(fp, "%s INFO  %s:%d:%s: ", timestr, file, line, func);
            break;
        case LogLevel_WARN:
            fprintf(fp, "%s WARN  %s:%d:%s: ", timestr, file, line, func);
            break;
        case LogLevel_ERROR:
            fprintf(fp, "%s ERROR %s:%d:%s: ", timestr, file, line, func);
            break;
        default:
            assert(0 && "Unknown LogLevel");
    }

    va_list list;
    va_start(list, fmt);
    vfprintf(fp, fmt, list);
    va_end(list);
}

void logger_setLevel(LogLevel newLevel)
{
    currentLevel = newLevel;
}

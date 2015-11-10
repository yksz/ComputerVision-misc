#include "logger.h"
#include <assert.h>
#include <stdarg.h>
#include <time.h>

static LogLevel s_logLevel = LogLevel_INFO;

void logger_setLevel(LogLevel level)
{
    s_logLevel = level;
}

void logger_log(LogLevel level, FILE* fp, const char* file, int line, const char* func, const char* fmt, ...)
{
    time_t now;
    char timestr[20];
    const char* levelstr;
    va_list arg;

    if (s_logLevel > level) {
        return;
    }

    now = time(NULL);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    switch (level) {
        case LogLevel_DEBUG:
            levelstr = "DEBUG";
            break;
        case LogLevel_INFO:
            levelstr = "INFO ";
            break;
        case LogLevel_WARN:
            levelstr = "WARN ";
            break;
        case LogLevel_ERROR:
            levelstr = "ERROR ";
            break;
        default:
            assert(0 && "Unknown LogLevel");
            return;
    }
    fprintf(fp, "%s %s %s:%d:%s: ", timestr, levelstr, file, line, func);

    va_start(arg, fmt);
    vfprintf(fp, fmt, arg);
    va_end(arg);

    fprintf(fp, "\n");
}

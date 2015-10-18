#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum
{
    LogLevel_DEBUG,
    LogLevel_INFO,
    LogLevel_WARN,
    LogLevel_ERROR,
} LogLevel;

#ifdef _WIN32
#include <string.h>
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define __func__ __FUNCTION__
#else
#define __FILENAME__ __FILE__
#endif

#define LOGGER_DEBUG(fmt, ...) logger_write(LogLevel_DEBUG, stderr, __FILENAME__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define LOGGER_INFO (fmt, ...) logger_write(LogLevel_INFO , stderr, __FILENAME__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define LOGGER_WARN (fmt, ...) logger_write(LogLevel_WARN , stderr, __FILENAME__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define LOGGER_ERROR(fmt, ...) logger_write(LogLevel_ERROR, stderr, __FILENAME__, __LINE__, __func__, fmt, ## __VA_ARGS__)

void logger_write(LogLevel level, FILE* fp, const char* file, int line, const char* func, const char* fmt, ...);
void logger_setLevel(LogLevel level);

#endif /* LOGGER_H */

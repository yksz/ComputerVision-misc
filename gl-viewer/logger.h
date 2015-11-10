#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#ifdef _WIN32
#include <string.h>
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define __func__ __FUNCTION__
#else
#define __FILENAME__ __FILE__
#endif

#define LOG_DEBUG(fmt, ...) logger_log(LogLevel_DEBUG, stderr, __FILENAME__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define LOG_INFO(fmt, ...)  logger_log(LogLevel_INFO , stderr, __FILENAME__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define LOG_WARN(fmt, ...)  logger_log(LogLevel_WARN , stderr, __FILENAME__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define LOG_ERROR(fmt, ...) logger_log(LogLevel_ERROR, stderr, __FILENAME__, __LINE__, __func__, fmt, ## __VA_ARGS__)

typedef enum
{
    LogLevel_DEBUG,
    LogLevel_INFO,
    LogLevel_WARN,
    LogLevel_ERROR,
} LogLevel;

void logger_setLevel(LogLevel level);
void logger_log(LogLevel level, FILE* fp, const char* file, int line, const char* func, const char* fmt, ...);

#endif /* LOGGER_H */

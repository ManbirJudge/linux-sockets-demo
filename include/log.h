#ifndef LOG_H
#define LOG_H

typedef enum {
    LOG_LVL_VERBOSE, 
    LOG_LVL_DEBUG, 
    LOG_LVL_INFO,
    LOG_LVL_WARN,
    LOG_LVL_ERROR,
    LOG_LVL_SUCCESS
} LogLevel;

void _log(LogLevel lvl, const char *fmt, ...);

#define LOG_V(...) _log(LOG_LVL_VERBOSE, __VA_ARGS__)
#define LOG_D(...) _log(LOG_LVL_DEBUG  , __VA_ARGS__)
#define LOG_I(...) _log(LOG_LVL_INFO   , __VA_ARGS__)
#define LOG_W(...) _log(LOG_LVL_WARN   , __VA_ARGS__)
#define LOG_E(...) _log(LOG_LVL_ERROR  , __VA_ARGS__)
#define LOG_S(...) _log(LOG_LVL_SUCCESS, __VA_ARGS__)

#endif
#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void _log(LogLevel lvl, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char lvl_tag[6];
    char ansi_esc_clr[20];
    switch (lvl) {
        case LOG_LVL_VERBOSE:
            strcpy(lvl_tag, "VERBO");
            strcpy(ansi_esc_clr, "\x1b[2m");
            break;
        case LOG_LVL_DEBUG:  
            strcpy(lvl_tag, "DEBUG");
            strcpy(ansi_esc_clr, "\x1b[37m");
            break;
        case LOG_LVL_INFO:   
            strcpy(lvl_tag, " INFO");
            strcpy(ansi_esc_clr, "\x1b[34m");
            break;
        case LOG_LVL_WARN:   
            strcpy(lvl_tag, " WARN");
            strcpy(ansi_esc_clr, "\x1b[33m");
            break;
        case LOG_LVL_ERROR:  
            strcpy(lvl_tag, "ERROR");
            strcpy(ansi_esc_clr, "\x1b[31m");
            break;
        case LOG_LVL_SUCCESS:  
            strcpy(lvl_tag, "SUCES");
            strcpy(ansi_esc_clr, "\x1b[32m");
            break;
    }


    printf("%s[%s] ", ansi_esc_clr, lvl_tag);
    vprintf(fmt, args);
    printf("\x1b[0m\n");

    va_end(args);
}
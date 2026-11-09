/**
 * @file        logger.c
 * @brief       Implementation of a Logger, and macros to easily log
 *              messages to it.
 * 
 * @date        Original: June, 2024. Update: November, 2026.
 * @author      Causse, Juan Ignacio (jcausse@itba.edu.ar)
 */

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>     // malloc(), free(), getenv()
#include <stdarg.h>     // va_list, va_start(), va_end()
#include <string.h>     // strcmp()
#include <time.h>       // time_t, time(), localtime(), strftime()

#if defined(_WIN32)
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static bool stream_is_tty(FILE * stream) {
    if (!stream) return false;
    return _isatty(_fileno(stream)) != 0;
}

static void try_enable_windows_vt(FILE * stream) {
    HANDLE hOut = (HANDLE)_get_osfhandle(_fileno(stream));
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
}
#else
#include <unistd.h>     // isatty(), fileno()

static bool stream_is_tty(FILE * stream) {
    if (!stream) return false;
    return isatty(fileno(stream)) != 0;
}
#endif

#define ANSI_RESET   "\033[0m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_RED     "\033[31;1m"

static const char * LogLevelColors[LOG_LEVELS_QTY] = {
    [LOGGER_LEVEL_DEBUG] = ANSI_CYAN,
    [LOGGER_LEVEL_INFO]  = ANSI_GREEN,
    [LOGGER_LEVEL_WARN]  = ANSI_YELLOW,
    [LOGGER_LEVEL_ERROR] = ANSI_RED
};

typedef struct _Logger_t {
    LogLevels       min_level;
    bool            with_datetime;
    bool            with_level;
    bool            use_color;
    bool            flush_immediately;
    const char *    log_prefix;

    /* File or Stream to write the logs */
    FILE *          log_file;
    bool            should_close;
} _Logger_t;

static const char * LogLevelsStr[] = {
    #define XX(log_level_numeric, log_level_ascii) log_level_ascii,
    LOG_LEVELS(XX)
    #undef XX
};

/**
 * @brief       Get the local time in format yyyy/mm/dd-HH:MM:SS.
 * 
 * @param[out] buff     Buffer to store the result.
 * @param[in]  size     Buffer size in bytes.
 */
static void get_current_datetime(char * buff, size_t size){
    // Get the current time
    time_t rawtime;
    struct tm * timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Format the date and time
    strftime(buff, size, DATETIME_FORMAT, timeinfo);
}

static bool should_enable_color(bool requested, FILE * stream) {
    if (!requested || !stream) {
        return false;
    }
    if (!stream_is_tty(stream)) {
        return false;
    }
    // Respect NO_COLOR standard (https://no-color.org)
    const char * no_color = getenv("NO_COLOR");
    if (no_color != nullptr && no_color[0] != '\0') {
        return false;
    }
    // Respect dumb terminal
    const char * term = getenv("TERM");
    if (term != nullptr && strcmp(term, "dumb") == 0) {
        return false;
    }
    return true;
}

static Logger Logger_create_common(LoggerConfig config, FILE * log_file, bool should_close){
    Logger self = malloc(sizeof(_Logger_t));
    if (!self) return nullptr;

    bool use_color = should_enable_color(config.with_color, log_file);
#if defined(_WIN32)
    if (use_color) {
        try_enable_windows_vt(log_file);
    }
#endif

    /* Initialize configuration */
    self->min_level         = config.min_log_level;
    self->with_datetime     = config.with_datetime;
    self->with_level        = config.with_level;
    self->use_color         = use_color;
    self->flush_immediately = config.flush_immediately;
    self->log_prefix        = config.log_prefix;
    self->log_file          = log_file;
    self->should_close      = should_close;

    /* Success */
    return self;
}

Logger Logger_create_from_stream(LoggerConfig config, FILE * const stream) {
    return Logger_create_common(config, stream, false);
}

Logger Logger_create_from_path(LoggerConfig config, const char * restrict file_path) {
    FILE * log_file = fopen(file_path, "a");
    if (!log_file) return nullptr;

    Logger self = Logger_create_common(config, log_file, true);
    if (!self) {
        fclose(log_file);
        return nullptr;
    };
    
    return self;
}

bool Logger_log(Logger const self, LogLevels level, const char * restrict fmt, ...){
    if (self == nullptr || level < 0 || level >= LOG_LEVELS_QTY || level < self->min_level || fmt == nullptr){
        return false;
    }

    bool did_print = false;

    /* Add log prefix */
    if (self->log_prefix != nullptr){
        fprintf(self->log_file, "[%s] ", self->log_prefix);
        did_print = true;
    }

    /* Add date and time */
    if (self->with_datetime){
        char datetime_buff[MAX_DATETIME_LEN] = {0};
        get_current_datetime(datetime_buff, MAX_DATETIME_LEN);
        fprintf(self->log_file, "@[%s] ", datetime_buff);
        did_print = true;
    }
    
    /* Add level */
    if (self->with_level){
        if (self->use_color && level >= 0 && level < LOG_LEVELS_QTY && LogLevelColors[level] != nullptr) {
            fprintf(self->log_file, "%s[%s]" ANSI_RESET " ", LogLevelColors[level], LogLevelsStr[level]);
        } else {
            fprintf(self->log_file, "[%s] ", LogLevelsStr[level]);
        }
        did_print = true;
    }
    
    /* Add separator if prefix, datetime or level were added before */
    if (did_print){
        fprintf(self->log_file, "--> ");
    }

    /* Add message */
    va_list ap;
    va_start(ap, fmt);
    vfprintf(self->log_file, fmt, ap);
    va_end(ap);

    /* Add new line at end of log line */
    fprintf(self->log_file, "\n");

    /* Perform fflush if flush_immediately is set */
    if (self->flush_immediately){
        fflush(self->log_file);
    }
    
    return true;
}

void Logger_destroy(Logger const self){
    if (self == nullptr){
        return;
    }
    if (self->log_file != nullptr){
        fflush(self->log_file);
        if (self->should_close) fclose(self->log_file);
    }
    free(self);
}

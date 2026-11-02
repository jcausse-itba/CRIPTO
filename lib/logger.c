#include "logger.h"
#include <stdlib.h>     // malloc(), free()
#include <stdarg.h>     // va_list, va_start(), va_end()
#include <time.h>       // time_t, time(), localtime(), strftime()

typedef struct _Logger_t {
    LogLevels       min_level;
    bool            with_datetime;
    bool            with_level;
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

static Logger Logger_create_common(LoggerConfig config, FILE * log_file, bool should_close){
    Logger self = malloc(sizeof(_Logger_t));
    if (!self) return nullptr;

    /* Initialize configuration */
    self->min_level         = config.min_log_level;
    self->with_datetime     = config.with_datetime;
    self->with_level        = config.with_level;
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
        fprintf(self->log_file, "[%s] ", LogLevelsStr[level]);
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

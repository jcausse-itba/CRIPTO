/**
 * @file        logger.h
 * @brief       Implementation of a Logger, and macros to easily log
 *              messages to it.
 * 
 * @note        For further optimization, to use the macro `LOG_DEBUG()`, 
 *              the constant `__USE_DEBUG_LOGS__` must be defined at compile-time.
 *              Independently of the value passed to `LoggerConfig.min_log_level`,
 *              said macro expands empty when `__USE_DEBUG_LOGS__` is not defined
 *              in order to optimize production executables.
 *              All direct calls to `Logger_log()` with `LOGGER_LEVEL_DEBUG` passed
 *              as `level` do log debug messages without said constant defined.
 * 
 * @date        June, 2024
 * @author      Causse, Juan Ignacio (jcausse@itba.edu.ar)
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdio.h>      // FILE, vfprintf()
#include <stdlib.h>     // malloc(), free()
#include <stdbool.h>    // bool, true, false
#include <stdarg.h>     // va_list, va_start(), va_end()
#include <unistd.h>     // close()
#include <errno.h>      // errno
#include <time.h>       // time_t, time(), localtime(), strftime()

/****************************************************************************************/
/* CUSTOMIZABLE                                                                         */
/****************************************************************************************/

/* Default minimum log level */
#define LOGGER_DEFAULT_MIN_LOG_LEVEL   LOGGER_LEVEL_INFO

/* Maximum formatted date and time length */
#define MAX_DATETIME_LEN 32

/* Date and time format as required by strftime (3). Formatted date and time must be MAX_DATETIME_LEN chars at most. */
#define DATETIME_FORMAT "%Y/%m/%d-%H:%M:%S"

/****************************************************************************************/
/* Macro and constant definitions                                                       */
/****************************************************************************************/

/**
 *              Logger macros. These macros provide easy access to the logger from any
 *              function. It is required to have a reference to the Logger to be used
 *              (that is, a variable of type Logger), called "logger" to use these macros.
 */
#ifdef __USE_DEBUG_LOGS__
#define LOG_DEBUG(...)     Logger_log(logger, LOGGER_LEVEL_DEBUG,    __VA_ARGS__)
#else // __USE_DEBUG_LOGS__ not defined
#define LOG_DEBUG(...)
#endif // __USE_DEBUG_LOGS__
#define LOG_INFO(...)      Logger_log(logger, LOGGER_LEVEL_INFO,     __VA_ARGS__)
#define LOG_WARN(...)      Logger_log(logger, LOGGER_LEVEL_WARN,     __VA_ARGS__)
#define LOG_ERROR(...)     Logger_log(logger, LOGGER_LEVEL_ERROR,    __VA_ARGS__)

/****************************************************************************************/
/* Custom data types and enumerations                                                   */
/****************************************************************************************/

/**
 * @typedef     Logger: main Logger data type.
 */
typedef struct _Logger_t * Logger;

/**
 * Usage of the different Logger Levels:
 * 
 *  LOGGER_LEVEL_DEBUG:     For debugging purposes.
 *  LOGGER_LEVEL_INFO:      For regular logs.
 *  LOGGER_LEVEL_WARN:      Default logging level.
 *  LOGGER_LEVEL_ERROR:     Errors only
 */
#define LOG_LEVELS(XX)                        \
    XX(LOGGER_LEVEL_DEBUG,      " DEBUG  "  ) \
    XX(LOGGER_LEVEL_INFO,       "  INFO  "  ) \
    XX(LOGGER_LEVEL_WARN,       "  WARN  "  ) \
    XX(LOGGER_LEVEL_ERROR,      " ERROR  "  )

/**
 * @enum        LogLevels: levels used for the Logger. Any logs with level less than
 *              `min_level` will be ignored.
 */
typedef enum {
    #define XX(log_level_numeric, log_level_ascii) log_level_numeric,
    LOG_LEVELS(XX)
    #undef XX
    LOG_LEVELS_QTY
} LogLevels;

/**
 * @typedef      LoggerConfig: Logger configuration. This structure is used to set varios
 *              parameters that customize the way the Logger behaves.
 */
typedef struct {
    LogLevels       min_log_level;      // Minimum level for a message to be logged.
    bool            with_datetime;      // Include date and time in logs.
    bool            with_level;         // Include log level in logs.
    bool            flush_immediately;  // Write log to the log file immediately after logging. Helps visualize logs
                                        // in real time when using tail -f command.
    const char *    log_prefix;         // Include a custom prefix string before log messages. nullptr for no prefix.
                                        // Note that the string is NOT internally copied, so it must either be in
                                        // the constants memory section or must be dynamically allocated and not free'd
                                        // while the Logger is in use.
} LoggerConfig;

/****************************************************************************************/

/**
 * @brief           Create a new Logger providing a managed FILE stream for the logs.
 *
 * @param[in] config        Logger configuration.
 * @param[in] stream        The stream where to write the logs (typically `stderr`).
 * 
 * @return          A Logger on success, nullptr on failure. On error, errno is set accordingly.
 */
Logger Logger_create_from_stream(LoggerConfig config, FILE * const stream);

/**
 * @brief           Create a new Logger providing a file path for the logs.
 *
 * @param[in] config        Logger configuration.
 * @param[in] file_path     The path to the file where the logs will be saved.
 * 
 * @return          A Logger on success, nullptr on failure. On error, errno is set accordingly.
 */
Logger Logger_create_from_path(LoggerConfig config, const char * const file_path);

/**
 * @brief           Log a message to the Logger.
 * 
 * @param[in] self          The Logger itself.
 * @param[in] level         The log level to use for the current message.
 * @param[in] fmt           Format string that allows using the same format specifiers
 *                          as `printf` (i.e. %d, %i, %lg, %s, etc.).
 * @param[in] ...           Variable arguments (values for each specifier used in `fmt`).
 * 
 * @return          true if the message was logged, false otherwise.
 */
bool Logger_log(Logger const self, LogLevels level, const char * __restrict__ fmt, ...);

/**
 * @brief           Cleanup the Logger (flush and close its open log file, and free its
 *                  allocated memory).
 * 
 * @param[in] self          The Logger itself.
 */
void Logger_cleanup(Logger const self);

#endif // __LOGGER_H__

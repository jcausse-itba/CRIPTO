#ifndef CLI_H
#define CLI_H

/**
 * @brief Maximum length for a short option name (including null terminator).
 */
#define CLI_MAX_SHORT_OPTION_NAME 4

/**
 * @brief Maximum length for a long option name (including null terminator).
 */
#define CLI_MAX_LONG_OPTION_NAME 64

//------------------------------------------------------------------------------------------------------------------------------------------

/**
 * @brief CLI parser instance.
 */
typedef struct cli_internal * cli;

/**
 * @brief Supported types for command line options.
 */
typedef enum {
    CLI_TYPE_INTEGER,   // Long integer type.
    CLI_TYPE_FLOAT,     // Double-precision floating point type.
    CLI_TYPE_STRING,    // String type (char *). */
    CLI_TYPE_PATH       // File path type, validated using a regex.
} cli_option_type;

/**
 * @brief Union holding the default value for an option.
 */
typedef union {
    long    long_value;     // Default value for integer options.
    double  double_value;   // Default value for float options.
    char *  string_value;   // Default value for string and path options.
} cli_option_value;

/**
 * @brief Definition of a single command line option.
 */
typedef struct {
    char                    option_name_short[CLI_MAX_SHORT_OPTION_NAME];  // Short option name, e.g., "a" or "-a".
    char                    option_name_long[CLI_MAX_LONG_OPTION_NAME];    // Long option name, e.g., "alpha" or "--alpha".
    cli_option_type         type;                               // Data type expected for the option's argument.
    bool                    required;                           // True if the option must be provided by the user.
    cli_option_value        default_value;                      // Default value if the option is not provided (unused if required is true).
} cli_option;

//------------------------------------------------------------------------------------------------------------------------------------------

/**
 * @brief Creates a new CLI parser object.
 * 
 * @return A new instance of the CLI parser, or NULL on memory allocation failure.
 */
cli cli_create();

/**
 * @brief Destroys a CLI parser object and frees associated resources.
 * 
 * @param c The CLI parser to destroy. If NULL, does nothing.
 */
void cli_destroy(cli c);

/**
 * @brief Adds an option specification to the CLI parser.
 * 
 * @param c The CLI parser.
 * @param o The option definition to add.
 * @return true on success, false if the parser is invalid or maximum options reached.
 */
bool cli_add_option(cli c, cli_option o);

/**
 * @brief Parses the provided command line arguments based on the added options.
 * 
 * Non-added options are not parsed and effectively ignored.
 * 
 * @param c The CLI parser.
 * @param argc The argument count.
 * @param argv The argument vector.
 * @return true if all required options are present and types match, false otherwise.
 */
bool cli_parse(cli c, int argc, const char *argv[]);

/**
 * @brief Retrieves the parsed integer value for a given option.
 * 
 * @param c The CLI parser.
 * @param name The short or long name of the option.
 * @param out_value Pointer to store the parsed value.
 * @return true if the option was set and is of type integer, false otherwise.
 */
bool cli_get_integer(cli c, const char *name, long *out_value);

/**
 * @brief Retrieves the parsed float value for a given option.
 * 
 * @param c The CLI parser.
 * @param name The short or long name of the option.
 * @param out_value Pointer to store the parsed value.
 * @return true if the option was set and is of type float, false otherwise.
 */
bool cli_get_float(cli c, const char *name, double *out_value);

/**
 * @brief Retrieves the parsed string or path value for a given option.
 * 
 * @param c The CLI parser.
 * @param name The short or long name of the option.
 * @param out_value Pointer to store the parsed string pointer.
 * @return true if the option was set and is of type string/path, false otherwise.
 */
bool cli_get_string(cli c, const char *name, const char **out_value);

#endif // CLI_H

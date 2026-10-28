/**
 * @file        cli.h
 * @brief       Command Line Arguments Parser.
 * @date        October, 2026.
*/

#ifndef CLI_H
#define CLI_H

/**
 * @brief Maximum amount of options supported by the CLI parser. This can be configured as needed,
 *        but the larger the number, the more memory the CLI parser consumes. Also, using a very
 *        high number can cause stack overflows.
 */
#define MAX_OPTIONS 32

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
    CLI_TYPE_STRING,    // String type (char *).
    CLI_TYPE_PATH,      // File path type, validated using a regex.
    CLI_TYPE_FLAG,      // Boolean flag type (no argument).
    CLI_TYPE_CHOICE     // String choice type, validated against a NULL-terminated array of allowed values.
} cli_option_type;

/**
 * @brief Error codes reported by the CLI parser when cli_parse fails.
 */
typedef enum {
    CLI_PARSE_ERROR_NONE = 0,           // No error occurred.
    CLI_PARSE_ERROR_UNKNOWN_OPTION,     // An unregistered option was encountered (when fail_on_unknown is true).
    CLI_PARSE_ERROR_MISSING_REQUIRED,   // A required option was not provided.
    CLI_PARSE_ERROR_MISSING_VALUE,      // An option requiring a value was passed without an argument.
    CLI_PARSE_ERROR_NOT_A_CHOICE,       // A CLI_TYPE_CHOICE option received a value not in its allowed choices.
    CLI_PARSE_ERROR_INVALID_VALUE,      // An option's value failed type or format validation (integer, float, path).
    CLI_PARSE_ERROR_OTHER               // Memory allocation failure or invalid parser state.
} cli_error_code;

/**
 * @brief Union holding the value for an option.
 */
typedef union {
    long            long_value;         // Value for integer options.
    double          double_value;       // Value for float options.
    const char *    string_value;       // Value for string, path, and choice options.
    bool            bool_value;         // Value for flag options (parsed value only; flags always default to false).
} cli_option_value;

/**
 * @brief Definition of a single command line option.
 *
 * @details Field rules and interactions when creating a @ref cli_option:
 * - **Option Names (`option_name_short` & `option_name_long`):**
 *   - Leading dashes (e.g., `"-a"`, `"--alpha"`) are optional and automatically stripped upon registration.
 *   - At least one of `option_name_short` or `option_name_long` must be non-empty (after stripping dashes).
 *   - `option_name_short`, if provided, must be a single character (after stripping dashes).
 *   - Both names must be unique across all registered options and cannot be identical to each other.
 *
 * - **Requirement & Default Values (`required` & `default_value`):**
 *   - When `required` is `true`: the option must be explicitly passed on the command line (otherwise `cli_parse`
 *     fails with @ref CLI_PARSE_ERROR_MISSING_REQUIRED), and `default_value` is ignored.
 *   - When `required` is `false`: if the option is omitted on the command line, `default_value` is used for
 *     @ref CLI_TYPE_INTEGER (`long_value`), @ref CLI_TYPE_FLOAT (`double_value`), and @ref CLI_TYPE_STRING /
 *     @ref CLI_TYPE_PATH / @ref CLI_TYPE_CHOICE (`string_value`). Note that default strings are referenced
 *     directly (not duplicated), so they must remain valid for the lifetime of the parser.
 *   - For @ref CLI_TYPE_FLAG: `default_value` is **always ignored**. Flags take no arguments on the command line,
 *     always default to `false` when omitted (if `required` is `false`), and evaluate to `true` when present.
 *
 * - **Choices (`choices`, `case_insensitive` & `type`):**
 *   - When `type` is @ref CLI_TYPE_CHOICE: `choices` **must** be a non-empty, `NULL`-terminated array of allowed
 *     strings (e.g., `(const char *[]){"AES128", "AES256", nullptr}`). Registration via `cli_add_option` fails if
 *     `choices` is `NULL` or empty. The array and its strings are not copied and must outlive the parser.
 *   - `case_insensitive` controls how user input is matched against `choices`: when `false` (the default if omitted),
 *     matching is case-sensitive; when `true`, matching is case-insensitive and the canonical string from `choices`
 *     is stored as the parsed value.
 *   - For all other option types: `choices` and `case_insensitive` are unused and should be omitted (`NULL` / `false`).
 */
typedef struct {
    char                    option_name_short[CLI_MAX_SHORT_OPTION_NAME];  // Short option name, e.g., "a" or "-a".
    char                    option_name_long[CLI_MAX_LONG_OPTION_NAME];    // Long option name, e.g., "alpha" or "--alpha".
    cli_option_type         type;                               // Data type expected for the option's argument.
    bool                    required;                           // True if the option must be provided by the user.
    cli_option_value        default_value;                      // Default value if the option is not provided (unused if required is true or for flags).
    const char **           choices;                            // NULL-terminated array of valid strings for CLI_TYPE_CHOICE options.
    bool                    case_insensitive;                   // If true, CLI_TYPE_CHOICE matching is case-insensitive (defaults to false, i.e., case-sensitive).
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
 * @param[in]   c       The CLI parser to destroy. If NULL, does nothing.
 */
void cli_destroy(cli c);

/**
 * @brief Adds an option specification to the CLI parser.
 * 
 * @param[in]   c       The CLI parser.
 * @param[in]   o       The option definition to add.
 * @return true on success, false if the parser is invalid or maximum options reached.
 */
bool cli_add_option(cli c, cli_option o);

/**
 * @brief Parses the provided command line arguments based on the added options.
 * 
 * @param[in]   c       The CLI parser.
 * @param[in]   argc    The argument count.
 * @param[in]   argv    The argument vector.
 * @param[in]   fail_on_unknown     If true, unknown options cause parsing to fail with CLI_PARSE_ERROR_UNKNOWN;
 *                                  if false, they are ignored.
 * @return true if all required options are present and types match, false otherwise.
 */
bool cli_parse(cli c, int argc, const char *argv[], bool fail_on_unknown);

/**
 * @brief Retrieves the error code and offending option name from the last cli_parse call.
 * 
 * @param[in]   c       The CLI parser.
 * @param[out]  option  Output pointer to receive the name of the option that caused the failure (or NULL if not applicable).
 * @return The cli_error_code describing the parse result.
 */
cli_error_code cli_get_error(cli c, char **option);

/**
 * @brief Generates and returns a usage string based on the registered options.
 * 
 * The string is allocated on demand when called and owned by the CLI parser (freed by cli_destroy).
 * 
 * @param[in]   c       The CLI parser.
 * @param[in]   prog_name   The program name to display in the usage synopsis (defaults to "program" if NULL or empty).
 * @return Pointer to the generated usage string, or NULL on failure.
 */
const char * cli_usage(cli c, const char *prog_name);

/**
 * @brief Checks whether a given option was explicitly provided on the command line.
 * 
 * @param[in]   c       The CLI parser.
 * @param[in]   name    The short or long name of the option.
 * @param[out]  out_is_set  Pointer to store true if the option was explicitly passed by the user, false if omitted/defaulted.
 * @return true if the option exists and cli_parse has been called, false otherwise.
 */
bool cli_is_set(cli c, const char *name, bool *out_is_set);

/**
 * @brief Retrieves the parsed integer value for a given option.
 * 
 * @param[in]   c       The CLI parser.
 * @param[in]   name    The short or long name of the option.
 * @param[out]  out_value   Pointer to store the parsed value.
 * @return true if the option has a value and is of type integer, false otherwise.
 */
bool cli_get_integer(cli c, const char *name, long *out_value);

/**
 * @brief Retrieves the parsed float value for a given option.
 * 
 * @param[in]   c       The CLI parser.
 * @param[in]   name    The short or long name of the option.
 * @param[out]  out_value   Pointer to store the parsed value.
 * @return true if the option has a value and is of type float, false otherwise.
 */
bool cli_get_float(cli c, const char *name, double *out_value);

/**
 * @brief Retrieves the parsed string, path, or choice value for a given option.
 * 
 * @param[in]   c       The CLI parser.
 * @param[in]   name    The short or long name of the option.
 * @param[out]  out_value   Pointer to store the parsed string pointer.
 * @return true if the option has a value and is of type string/path/choice, false otherwise.
 */
bool cli_get_string(cli c, const char *name, const char **out_value);

/**
 * @brief Retrieves the parsed boolean value for a given flag option.
 * 
 * @param[in]   c       The CLI parser.
 * @param[in]   name    The short or long name of the option.
 * @param[out]  out_value   Pointer to store the parsed boolean value.
 * @return true if the option has a value and is of type flag, false otherwise.
 */
bool cli_get_flag(cli c, const char *name, bool *out_value);

#endif // CLI_H

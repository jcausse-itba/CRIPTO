/**
 * @file        cli.c
 * @brief       Command Line Arguments Parser.
 * @date        October, 2026.
*/

#include "cli.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <regex.h>
#include <errno.h>

#define MAX_OPTIONS 128
#define USAGE_CHUNK_SIZE 50

#define PATH_REGEX "^[a-zA-Z0-9_./\\\\-]+$"

typedef struct {
    cli_option def;
    bool is_set;                    // true only if explicitly provided on the command line
    bool owns_string;               // true if parsed_value.string_value was strdup'd and must be freed
    cli_option_value parsed_value;
} cli_option_internal;

struct cli_internal {
    cli_option_internal options[MAX_OPTIONS];
    size_t option_count;
    bool is_parsed;
    bool path_regex_compiled;
    regex_t path_regex;
    cli_error_code error_code;
    char error_option[CLI_MAX_LONG_OPTION_NAME];
    char * usage_str;
    HashMap map;
};

/*************************************************************************/
/*                          HashMap callbacks                            */
/*************************************************************************/

static uint32_t string_hash_fn(HashMapKey key, size_t hash_table_size) {
    const char *str = key;
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char) *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % hash_table_size;
}

static bool string_equals_fn(HashMapKey key1, HashMapKey key2) {
    if (key1 == key2) return true;
    if (!key1 || !key2) return false;
    return strncmp(key1, key2, CLI_MAX_LONG_OPTION_NAME) == 0;
}

/*************************************************************************/
/*                          Static helpers                               */
/*************************************************************************/

/**
 * @brief Strip leading dashes from a name buffer in-place.
 *
 * Returns the resulting string length, or 0 if the buffer was empty or
 * contained only dashes (in which case the buffer is set to "").
 */
static size_t strip_leading_dashes(char *buf, size_t buf_size) {
    if (buf_size == 0 || buf[0] == '\0') return 0;

    size_t start = 0;
    while (start < buf_size && buf[start] == '-') {
        start++;
    }

    const size_t remaining = strlen(buf + start);
    if (start > 0) {
        memmove(buf, buf + start, remaining + 1);
    }
    return remaining;
}

/**
 * @brief Return the canonical name of an option (prefers long name, falls back to short name).
 */
static const char * get_option_name(const cli_option_internal * opt) {
    if (opt->def.option_name_long[0] != '\0') {
        return opt->def.option_name_long;
    }
    return opt->def.option_name_short;
}

/**
 * @brief Record the first parse error and the offending option name.
 */
static void set_error(cli c, cli_error_code code, const char * option_name) {
    if (!c || c->error_code != CLI_PARSE_ERROR_NONE) return;
    c->error_code = code;
    if (option_name) {
        while (*option_name == '-') {
            option_name++;
        }
        size_t len = 0;
        while (option_name[len] != '\0' && option_name[len] != '=' && len < CLI_MAX_LONG_OPTION_NAME - 1) {
            c->error_option[len] = option_name[len];
            len++;
        }
        c->error_option[len] = '\0';
    } 
    else {
        c->error_option[0] = '\0';
    }
}

/**
 * @brief Attempt to parse an optarg as an integer.
 * @return true on success, false if the string is empty, overflows, or is not a valid integer.
 */
static bool parse_integer(const char * str, long * out) {
    if (!str || *str == '\0') return false;
    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);
    if (endptr == str || *endptr != '\0' || errno == ERANGE) return false;
    *out = val;
    return true;
}

/**
 * @brief Attempt to parse an optarg as a double.
 * @return true on success, false if the string is empty, overflows, or is not a valid double.
 */
static bool parse_float(const char * str, double * out) {
    if (!str || *str == '\0') return false;
    char *endptr;
    errno = 0;
    double val = strtod(str, &endptr);
    if (endptr == str || *endptr != '\0' || errno == ERANGE) return false;
    *out = val;
    return true;
}

/**
 * @brief Validate a path string against the cached path regex (compiled lazily once per CLI instance).
 * @return true if the path matches, false otherwise.
 */
static bool validate_path(cli c, const char * str) {
    if (!c->path_regex_compiled) {
        if (regcomp(&c->path_regex, PATH_REGEX, REG_EXTENDED | REG_NOSUB) != 0) return false;
        c->path_regex_compiled = true;
    }
    return regexec(&c->path_regex, str, 0, nullptr, 0) == 0;
}

/**
 * @brief Compare two strings case-insensitively.
 * @details     This is needed because strcasecmp is POSIX, not standard ISO C.
 */
static bool str_case_eq (const char * a, const char * b) {
    while (*a && *b) {
        if (tolower((unsigned char) *a) != tolower((unsigned char) *b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/**
 * @brief Compare two strings either case-sensitively or case-insensitively.
 */
static bool str_equals(const char * a, const char * b, bool case_insensitive) {
    return case_insensitive ? str_case_eq(a, b) : strcmp(a, b) == 0;
}

/**
 * @brief Match a string against a NULL-terminated array of allowed choices.
 * @return Pointer to the matched canonical choice string from the array, or NULL if no match.
 */
static const char * match_choice(const char * str, const char ** choices, bool case_insensitive) {
    if (!str || !choices) return nullptr;
    for (size_t i = 0; choices[i] != nullptr; i++) {
        if (str_equals(str, choices[i], case_insensitive)) {
            return choices[i];
        }
    }
    return nullptr;
}

/**
 * @brief Assign a long value to an option.
 */
static bool set_long_value(cli_option_internal * opt, const char * arg) {
    long val;
    if (!parse_integer(arg, &val)) return false;
    opt->parsed_value.long_value = val;
    opt->is_set = true;
    return true;
}

/**
 * @brief Assign a double value to an option.
 */
static bool set_double_value(cli_option_internal * opt, const char * arg) {
    double val;
    if (!parse_float(arg, &val)) return false;
    opt->parsed_value.double_value = val;
    opt->is_set = true;
    return true;
}

/**
 * @brief Assign a string value to an option, freeing any previously owned string.
 * @return true on success, false if strdup fails (OOM).
 */
static bool set_string_value(cli_option_internal * opt, const char * arg, bool owned) {
    const char * value = owned ? strdup(arg) : arg;
    if (owned && !value) return false;
    if (opt->owns_string && opt->parsed_value.string_value) {
        free((void *) opt->parsed_value.string_value);
    }
    opt->parsed_value.string_value = value;
    opt->owns_string = owned;
    if (owned) {
        opt->is_set = true;
    }
    return true;
}

/**
 * @brief Assign a boolean value to a flag option.
 */
static bool set_bool_value(cli_option_internal * opt, bool val) {
    opt->parsed_value.bool_value = val;
    opt->is_set = true;
    return true;
}

/**
 * @brief Parse and assign optarg to the matched option according to its type.
 * @return true on success, false on type mismatch or validation error.
 */
static bool assign_optarg(cli c, cli_option_internal * opt, const char * arg) {
    const char * name = get_option_name(opt);
    if (opt->def.type != CLI_TYPE_FLAG && !arg) {
        set_error(c, CLI_PARSE_ERROR_MISSING_VALUE, name);
        return false;
    }

    switch (opt->def.type) {
        case CLI_TYPE_INTEGER: {
            if (!set_long_value(opt, arg)) {
                set_error(c, CLI_PARSE_ERROR_INVALID_VALUE, name);
                return false;
            }
            return true;
        }
        case CLI_TYPE_FLOAT: {
            if (!set_double_value(opt, arg)) {
                set_error(c, CLI_PARSE_ERROR_INVALID_VALUE, name);
                return false;
            }
            return true;
        }
        case CLI_TYPE_STRING: {
            if (!set_string_value(opt, arg, true)) {
                set_error(c, CLI_PARSE_ERROR_OTHER, name);
                return false;
            }
            return true;
        }
        case CLI_TYPE_PATH: {
            if (!validate_path(c, arg)) {
                set_error(c, CLI_PARSE_ERROR_INVALID_VALUE, name);
                return false;
            }
            if (!set_string_value(opt, arg, true)) {
                set_error(c, CLI_PARSE_ERROR_OTHER, name);
                return false;
            }
            return true;
        }
        case CLI_TYPE_FLAG: {
            return set_bool_value(opt, true);
        }
        case CLI_TYPE_CHOICE: {
            const char * matched = match_choice(arg, opt->def.choices, opt->def.case_insensitive);
            if (!matched) {
                set_error(c, CLI_PARSE_ERROR_NOT_A_CHOICE, name);
                return false;
            }
            set_string_value(opt, matched, false);
            opt->is_set = true;     // Mark opt as set, even though we do not own the value
            return true;
        }
    }
    return false;
}

/**
 * @brief Apply the default value to an unset optional option without marking it as user-set.
 */
static void apply_default(cli_option_internal *opt) {
    switch (opt->def.type) {
        case CLI_TYPE_INTEGER:
            opt->parsed_value.long_value = opt->def.default_value.long_value;
            break;
        case CLI_TYPE_FLOAT:
            opt->parsed_value.double_value = opt->def.default_value.double_value;
            break;
        case CLI_TYPE_FLAG:
            opt->parsed_value.bool_value = false;
            break;
        case CLI_TYPE_STRING:
        /* fallthrough */
        case CLI_TYPE_PATH:
        /* fallthrough */
        case CLI_TYPE_CHOICE:
            // Points to the caller's string — not owned by us
            set_string_value(opt, opt->def.default_value.string_value, false);
            break;
    }
}

/**
 * @brief Look up an option in the hashmap by name, ignoring any leading dashes.
 * @return Pointer to the internal option, or NULL if not found.
 */
static cli_option_internal * lookup_option(cli c, const char * name) {
    if (!c || !name) return nullptr;
    while (*name == '-') {
        name++;
    }
    if (*name == '\0') return nullptr;
    cli_option_internal * opt = nullptr;
    if (HashMap_peek(c->map, (HashMapKey) name, (HashMapValue *) &opt) == HASHMAP_OK) {
        return opt;
    }
    return nullptr;
}

/**
 * @brief Check whether an option has a valid value available to retrieve.
 */
static bool has_value(cli c, const cli_option_internal * opt) {
    if (!c || !c->is_parsed || !opt) return false;
    return opt->is_set || !opt->def.required;
}

/**
 * @brief Build the getopt_long_only option tables from registered options.
 *
 * @param c              The CLI parser.
 * @param long_options   Output: caller-allocated array of size (MAX_OPTIONS + 1).
 * @param long_opt_map   Output: maps long_options index directly to cli_option_internal pointer.
 * @param short_options  Output: caller-allocated buffer of size (MAX_OPTIONS * 3 + 2).
 */
static void build_getopt_tables(
    cli c,
    struct option * long_options,
    cli_option_internal ** long_opt_map,
    char * short_options
) {
    int long_idx = 0;
    int short_idx = 0;

    short_options[short_idx++] = ':'; // return ':' for missing arguments

    for (size_t i = 0; i < c->option_count; i++) {
        cli_option_internal * opt = &c->options[i];
        const cli_option * def = &opt->def;
        const bool is_flag = (def->type == CLI_TYPE_FLAG);

        if (def->option_name_long[0] != '\0') {
            long_options[long_idx].name     = def->option_name_long;
            long_options[long_idx].has_arg  = is_flag ? no_argument : required_argument;
            long_options[long_idx].flag     = nullptr;
            long_options[long_idx].val      = 0;
            long_opt_map[long_idx]          = opt;
            long_idx++;
        }

        if (def->option_name_short[0] != '\0') {
            short_options[short_idx++] = def->option_name_short[0];
            if (!is_flag) {
                short_options[short_idx++] = ':';
            }
        }
    }

    // Zero only the sentinel entry instead of calloc-ing the entire table
    long_options[long_idx] = (struct option){0};
    short_options[short_idx] = '\0';
}

/**
 * @brief Append a string slice to a dynamically grown buffer in USAGE_CHUNK_SIZE increments.
 * @return true on success, false on reallocation failure.
 */
static bool usage_append(char ** buf, size_t * len, size_t * cap, const char * str) {
    const size_t add_len = strlen(str);
    if (*len + add_len + 1 > *cap) {
        size_t new_cap = *cap;
        while (*len + add_len + 1 > new_cap) {
            new_cap += USAGE_CHUNK_SIZE;
        }
        char * new_buf = realloc(*buf, new_cap);
        if (!new_buf) return false;
        *buf = new_buf;
        *cap = new_cap;
    }
    memcpy(*buf + *len, str, add_len);
    *len += add_len;
    (*buf)[*len] = '\0';
    return true;
}

/**
 * @brief Build the usage string in a single pass using chunked reallocations.
 * @return Newly allocated usage string, or NULL on allocation failure.
 */
static char * build_usage_string(cli c, const char * prog_name) {
    const char * prog = (prog_name && prog_name[0] != '\0') ? prog_name : "program";
    char * buf = nullptr;
    size_t len = 0;
    size_t cap = 0;

    #define APPEND_STR(s) \
        do { \
            if (!usage_append(&buf, &len, &cap, (s))) { \
                free(buf); \
                return nullptr; \
            } \
        } while (0)

    APPEND_STR("Usage: ");
    APPEND_STR(prog);

    for (size_t i = 0; i < c->option_count; i++) {
        const cli_option * def = &c->options[i].def;
        const bool has_short = def->option_name_short[0] != '\0';
        const bool has_long  = def->option_name_long[0] != '\0';

        APPEND_STR(def->required ? " " : " [");
        if (has_short && has_long) {
            APPEND_STR("-");
            APPEND_STR(def->option_name_short);
            APPEND_STR("|--");
            APPEND_STR(def->option_name_long);
        } 
        else if (has_short) {
            APPEND_STR("-");
            APPEND_STR(def->option_name_short);
        } 
        else {
            APPEND_STR("--");
            APPEND_STR(def->option_name_long);
        }

        switch (def->type) {
            case CLI_TYPE_INTEGER:
                APPEND_STR(" <int>");
                break;
            case CLI_TYPE_FLOAT:
                APPEND_STR(" <float>");
                break;
            case CLI_TYPE_STRING:
                APPEND_STR(" <string>");
                break;
            case CLI_TYPE_PATH:
                APPEND_STR(" <path>");
                break;
            case CLI_TYPE_FLAG:
                break;
            case CLI_TYPE_CHOICE:
                APPEND_STR(" <");
                if (def->choices) {
                    for (size_t j = 0; def->choices[j] != nullptr; j++) {
                        if (j > 0) APPEND_STR("|");
                        APPEND_STR(def->choices[j]);
                    }
                }
                APPEND_STR(">");
                break;
        }
        if (!def->required) {
            APPEND_STR("]");
        }
    }

    APPEND_STR("\n");

    #undef APPEND_STR
    return buf;
}

/*************************************************************************/
/*                          Public API                                   */
/*************************************************************************/

cli cli_create() {
    cli c = calloc(1, sizeof(struct cli_internal));
    if (!c) return nullptr;

    c->map = HashMap_create(string_hash_fn, string_equals_fn);
    if (!c->map) {
        free(c);
        return nullptr;
    }
    return c;
}

void cli_destroy(cli c) {
    if (!c) return;

    for (size_t i = 0; i < c->option_count; i++) {
        cli_option_internal * opt = &c->options[i];
        if (opt->owns_string && opt->parsed_value.string_value) {
            free((void *) opt->parsed_value.string_value);
        }
    }

    if (c->path_regex_compiled) {
        regfree(&c->path_regex);
    }

    if (c->usage_str) {
        free(c->usage_str);
    }

    if (c->map) {
        HashMap_cleanup(c->map, nullptr);
    }
    free(c);
}

bool cli_add_option(cli c, cli_option o) {
    if (!c || c->option_count >= MAX_OPTIONS) return false;

    // Choice options must provide a non-empty NULL-terminated array of allowed choices
    if (o.type == CLI_TYPE_CHOICE && (!o.choices || !o.choices[0])) return false;

    // Strip leading dashes — returns 0 for empty or all-dashes names
    const size_t short_len = strip_leading_dashes(o.option_name_short, CLI_MAX_SHORT_OPTION_NAME);
    const size_t long_len  = strip_leading_dashes(o.option_name_long, CLI_MAX_LONG_OPTION_NAME);

    // Short option name must be at most 1 character, and at least one name must be non-empty
    if (short_len > 1 || (short_len == 0 && long_len == 0)) return false;

    // Reject short/long collision within the same option
    if (short_len > 0 && long_len > 0 && strcmp(o.option_name_short, o.option_name_long) == 0) return false;

    cli_option_internal * opt = &c->options[c->option_count];
    opt->def = o;
    opt->is_set = false;
    opt->owns_string = false;

    // Register both name forms in the hashmap (HashMap_put rejects duplicates in a single traversal)
    if (short_len > 0) {
        if (HashMap_put(c->map, (HashMapKey) opt->def.option_name_short, (HashMapValue) opt) != HASHMAP_OK) {
            return false;
        }
    }
    if (long_len > 0) {
        if (HashMap_put(c->map, (HashMapKey) opt->def.option_name_long, (HashMapValue) opt) != HASHMAP_OK) {
            if (short_len > 0) {
                HashMap_pop(c->map, (HashMapKey) opt->def.option_name_short, nullptr);
            }
            return false;
        }
    }

    c->option_count++;
    return true;
}

bool cli_parse(cli c, int argc, const char *argv[], bool fail_on_unknown) {
    if (!c) return false;

    c->error_code = CLI_PARSE_ERROR_NONE;
    c->error_option[0] = '\0';

    if (!argv || argc < 1) {
        set_error(c, CLI_PARSE_ERROR_OTHER, nullptr);
        return false;
    }

    for (size_t i = 0; i < c->option_count; i++) {
        c->options[i].is_set = false;
    }

    // getopt_long_only only permutes the argv pointer array, not the strings themselves
    char **argv_copy = malloc((size_t)argc * sizeof(char *));
    if (!argv_copy) {
        set_error(c, CLI_PARSE_ERROR_OTHER, nullptr);
        return false;
    }
    memcpy(argv_copy, argv, (size_t)argc * sizeof(char *));

    struct option long_options[MAX_OPTIONS + 1];
    cli_option_internal * long_opt_map[MAX_OPTIONS];
    char short_options[MAX_OPTIONS * 3 + 2];
    build_getopt_tables(c, long_options, long_opt_map, short_options);

    // Save and configure getopt state
    int old_opterr = opterr;
    opterr = 0;
    optind = 1;

    bool success = true;
    int opt_val;
    int option_index = 0;

    while ((opt_val = getopt_long_only(argc, argv_copy, short_options, long_options, &option_index)) != -1) {
        if (opt_val == '?') {
            if (fail_on_unknown) {
                const char * bad_tok = (optind > 1 && optind <= argc) ? argv_copy[optind - 1] : nullptr;
                char fallback[2] = {(char)optopt, '\0'};
                set_error(c, CLI_PARSE_ERROR_UNKNOWN_OPTION, (bad_tok && bad_tok[0] == '-') ? bad_tok : (optopt ? fallback : nullptr));
                success = false;
                break;
            }
            continue;   // Ignore unknown option
        }
        if (opt_val == ':') {           // Missing required argument
            cli_option_internal * missing_opt = nullptr;
            if (optopt != 0) {
                char short_name[2] = {(char)optopt, '\0'};
                missing_opt = lookup_option(c, short_name);
            }
            if (!missing_opt && optind > 1 && optind <= argc) {
                missing_opt = lookup_option(c, argv_copy[optind - 1]);
            }
            const char * tok = (optind > 1 && optind <= argc) ? argv_copy[optind - 1] : nullptr;
            set_error(c, CLI_PARSE_ERROR_MISSING_VALUE, missing_opt ? get_option_name(missing_opt) : tok);
            success = false;
            break;
        }

        // Resolve the matched option: O(1) direct array access for long options, HashMap for short options
        cli_option_internal * found = nullptr;
        if (opt_val == 0) {
            found = long_opt_map[option_index];
        } 
        else {
            char short_name[2] = {(char)opt_val, '\0'};
            found = lookup_option(c, short_name);
        }

        if (found && !assign_optarg(c, found, optarg)) {
            success = false;
        }
    }

    opterr = old_opterr;

    // Apply defaults for unset options; fail on missing required ones
    for (size_t i = 0; i < c->option_count; i++) {
        if (!c->options[i].is_set) {
            if (c->options[i].def.required) {
                set_error(c, CLI_PARSE_ERROR_MISSING_REQUIRED, get_option_name(&c->options[i]));
                success = false;
            } 
            else {
                apply_default(&c->options[i]);
            }
        }
    }

    c->is_parsed = true;

    free(argv_copy);

    return success;
}

cli_error_code cli_get_error(cli c, char ** option) {
    if (!c) {
        if (option) *option = nullptr;
        return CLI_PARSE_ERROR_OTHER;
    }
    if (option) {
        *option = c->error_option[0] != '\0' ? c->error_option : nullptr;
    }
    return c->error_code;
}

const char * cli_usage(cli c, const char * prog_name) {
    if (!c) return nullptr;

    if (c->usage_str) {
        free(c->usage_str);
    }

    c->usage_str = build_usage_string(c, prog_name);
    return c->usage_str;
}

bool cli_is_set(cli c, const char * name, bool * out_is_set) {
    cli_option_internal * opt = lookup_option(c, name);
    if (!opt || !c->is_parsed) return false;
    if (out_is_set) *out_is_set = opt->is_set;
    return true;
}

bool cli_get_integer(cli c, const char * name, long * out_value) {
    cli_option_internal * opt = lookup_option(c, name);
    if (!has_value(c, opt) || opt->def.type != CLI_TYPE_INTEGER) return false;
    if (out_value) *out_value = opt->parsed_value.long_value;
    return true;
}

bool cli_get_float(cli c, const char * name, double * out_value) {
    cli_option_internal * opt = lookup_option(c, name);
    if (!has_value(c, opt) || opt->def.type != CLI_TYPE_FLOAT) return false;
    if (out_value) *out_value = opt->parsed_value.double_value;
    return true;
}

bool cli_get_string(cli c, const char * name, const char ** out_value) {
    cli_option_internal * opt = lookup_option(c, name);
    if (!has_value(c, opt)) return false;
    if (opt->def.type != CLI_TYPE_STRING && opt->def.type != CLI_TYPE_PATH && opt->def.type != CLI_TYPE_CHOICE) return false;
    if (out_value) *out_value = opt->parsed_value.string_value;
    return true;
}

bool cli_get_flag(cli c, const char * name, bool * out_value) {
    cli_option_internal * opt = lookup_option(c, name);
    if (!has_value(c, opt) || opt->def.type != CLI_TYPE_FLAG) return false;
    if (out_value) *out_value = opt->parsed_value.bool_value;
    return true;
}

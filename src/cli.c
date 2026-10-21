#include "cli.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <regex.h>
#include <errno.h>

#define MAX_OPTIONS 128

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
    cli_error_code error_code;
    char error_option[CLI_MAX_LONG_OPTION_NAME];
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

    if (start == 0) {
        return strlen(buf);
    }

    // buf[start] is either '\0' (all-dashes) or the first real char.
    // In either case memmove produces a valid C string.
    const size_t remaining = strlen(buf + start);
    memmove(buf, buf + start, remaining + 1);
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
 * @brief Validate a path string against the path regex.
 * @return true if the path matches, false otherwise.
 */
static bool validate_path(const char * str) {
    regex_t regex;
    if (regcomp(&regex, PATH_REGEX, REG_EXTENDED | REG_NOSUB) != 0) return false;
    bool ok = regexec(&regex, str, 0, nullptr, 0) == 0;
    regfree(&regex);
    return ok;
}

/**
 * @brief Validate a string against a NULL-terminated array of allowed choices.
 * @return true if the string matches one of the choices, false otherwise.
 */
static bool validate_choice(const char * str, const char ** choices) {
    if (!str || !choices) return false;
    for (size_t i = 0; choices[i] != nullptr; i++) {
        if (strcmp(str, choices[i]) == 0) {
            return true;
        }
    }
    return false;
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
    char * value = owned ? strdup(arg) : (char *) arg;
    if (owned && !value) return false;
    if (opt->owns_string && opt->parsed_value.string_value) {
        free(opt->parsed_value.string_value);
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
                set_error(c, CLI_PARSE_ERROR_MEMORY, name);
                return false;
            }
            return true;
        }
        case CLI_TYPE_PATH: {
            if (!validate_path(arg)) {
                set_error(c, CLI_PARSE_ERROR_INVALID_VALUE, name);
                return false;
            }
            if (!set_string_value(opt, arg, true)) {
                set_error(c, CLI_PARSE_ERROR_MEMORY, name);
                return false;
            }
            return true;
        }
        case CLI_TYPE_FLAG: {
            return set_bool_value(opt, true);
        }
        case CLI_TYPE_CHOICE: {
            if (!validate_choice(arg, opt->def.choices)) {
                set_error(c, CLI_PARSE_ERROR_NOT_A_CHOICE, name);
                return false;
            }
            if (!set_string_value(opt, arg, true)) {
                set_error(c, CLI_PARSE_ERROR_MEMORY, name);
                return false;
            }
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
 * @param long_options   Output: caller-allocated array of size (option_count + 1).
 * @param short_options  Output: caller-allocated buffer of size (MAX_OPTIONS * 3 + 2).
 */
static void build_getopt_tables(cli c, struct option * long_options, char * short_options) {
    int long_idx = 0;
    int short_idx = 0;

    short_options[short_idx++] = ':'; // return ':' for missing arguments

    for (size_t i = 0; i < c->option_count; i++) {
        const cli_option * def = &(c->options[i].def);
        const bool is_flag = (def->type == CLI_TYPE_FLAG);

        if (def->option_name_long[0] != '\0') {
            long_options[long_idx].name     = def->option_name_long;
            long_options[long_idx].has_arg  = is_flag ? no_argument : required_argument;
            long_options[long_idx].flag     = nullptr;
            long_options[long_idx].val      = def->option_name_short[0] != '\0'
                                              ? (unsigned char)def->option_name_short[0]
                                              : 0;
            long_idx++;
        }

        if (def->option_name_short[0] != '\0') {
            short_options[short_idx++] = def->option_name_short[0];
            if (!is_flag) {
                short_options[short_idx++] = ':';
            }
        }
    }

    short_options[short_idx] = '\0';
    // Sentinel entry for long_options (already zeroed by calloc)
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
            free(opt->parsed_value.string_value);
        }
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

    // Reject duplicate option names (including short/long collision within the same option)
    if (short_len > 0 && long_len > 0 && strcmp(o.option_name_short, o.option_name_long) == 0) return false;
    if (short_len > 0 && HashMap_contains(c->map, o.option_name_short)) return false;
    if (long_len > 0 && HashMap_contains(c->map, o.option_name_long)) return false;

    cli_option_internal * opt = &c->options[c->option_count];
    opt->def = o;
    opt->is_set = false;
    opt->owns_string = false;

    // Register both name forms in the hashmap
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
        set_error(c, CLI_PARSE_ERROR_MEMORY, nullptr);
        return false;
    }

    for (size_t i = 0; i < c->option_count; i++) {
        c->options[i].is_set = false;
    }

    // getopt_long_only may permute argv, so work on a mutable copy
    char **argv_copy = malloc(argc * sizeof(char *));
    if (!argv_copy) {
        set_error(c, CLI_PARSE_ERROR_MEMORY, nullptr);
        return false;
    }
    for (int i = 0; i < argc; i++) {
        argv_copy[i] = strdup(argv[i]);
        if (!argv_copy[i]) {
            for (int j = 0; j < i; j++) free(argv_copy[j]);
            free(argv_copy);
            set_error(c, CLI_PARSE_ERROR_MEMORY, nullptr);
            return false;
        }
    }

    struct option * long_options = calloc(c->option_count + 1, sizeof(struct option));
    if (!long_options) {
        for (int i = 0; i < argc; i++) free(argv_copy[i]);
        free(argv_copy);
        set_error(c, CLI_PARSE_ERROR_MEMORY, nullptr);
        return false;
    }

    char short_options[MAX_OPTIONS * 3 + 2];
    build_getopt_tables(c, long_options, short_options);

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
                set_error(c, CLI_PARSE_ERROR_UNKNOWN, (bad_tok && bad_tok[0] == '-') ? bad_tok : (optopt ? fallback : nullptr));
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

        // Resolve the matched option via the hashmap
        cli_option_internal * found = nullptr;
        if (opt_val == 0) {
            // Long-only option (no short alias): use name from long_options table
            found = lookup_option(c, long_options[option_index].name);
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

    free(long_options);
    for (int i = 0; i < argc; i++) free(argv_copy[i]);
    free(argv_copy);

    return success;
}

cli_error_code cli_get_error(cli c, char ** option) {
    if (!c) {
        if (option) *option = nullptr;
        return CLI_PARSE_ERROR_MEMORY;
    }
    if (option) {
        *option = c->error_option[0] != '\0' ? c->error_option : nullptr;
    }
    return c->error_code;
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

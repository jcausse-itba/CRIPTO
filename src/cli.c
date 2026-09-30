#include "cli.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <regex.h>

#define MAX_OPTIONS 128

#define PATH_REGEX "^[a-zA-Z0-9_./\\\\-]+$"

typedef struct {
    cli_option def;
    bool is_set;
    bool owns_string;               // true if parsed_value.string_value was strdup'd and must be freed
    union {
        long long_value;
        double double_value;
        char * string_value;
    } parsed_value;
} cli_option_internal;

struct cli_internal {
    cli_option_internal options[MAX_OPTIONS];
    size_t option_count;
    HashMap map;
};

/*************************************************************************/
/*                          HashMap callbacks                            */
/*************************************************************************/

static uint32_t string_hash_fn(const HashMapKey key, const size_t hash_table_size) {
    const char *str = key;
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % hash_table_size;
}

static bool string_equals_fn(const HashMapKey key1, const HashMapKey key2) {
    if (!key1 || !key2) return false;
    return strcmp(key1, key2) == 0;
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
    size_t remaining = strlen(buf + start);
    memmove(buf, buf + start, remaining + 1);
    return remaining;
}

/**
 * @brief Attempt to parse an optarg as an integer.
 * @return true on success, false if the string is not a valid integer.
 */
static bool parse_integer(const char * str, long * out) {
    char *endptr;
    long val = strtol(str, &endptr, 10);
    if (*endptr != '\0') return false;
    *out = val;
    return true;
}

/**
 * @brief Attempt to parse an optarg as a double.
 * @return true on success, false if the string is not a valid double.
 */
static bool parse_float(const char * str, double * out) {
    char *endptr;
    double val = strtod(str, &endptr);
    if (*endptr != '\0') return false;
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
    bool ok = regexec(&regex, str, 0, NULL, 0) == 0;
    regfree(&regex);
    return ok;
}

/**
 * @brief Assign a long value to an option.
 */
static bool set_long_value(cli_option_internal * opt, const char * optarg) {
    long val;
    if (!parse_integer(optarg, &val)) return false;
    opt->parsed_value.long_value = val;
    opt->is_set = true;
    return true;
}

/**
 * @brief Assign a double value to an option.
 */
static bool set_double_value(cli_option_internal * opt, const char * optarg) {
    double val;
    if (!parse_float(optarg, &val)) return false;
    opt->parsed_value.double_value = val;
    opt->is_set = true;
    return true;
}

/**
 * @brief Assign a string value to an option, freeing any previously owned string.
 * @return true on success, false if strdup fails (OOM).
 */
static bool set_string_value(cli_option_internal * opt, const char * optarg, bool owned) {
    char * value = owned ? strdup(optarg) : (char *) optarg;
    if (owned && !value) return false;
    if (opt->owns_string && opt->parsed_value.string_value) {
        free(opt->parsed_value.string_value);
    }
    opt->parsed_value.string_value = value;
    opt->owns_string = owned;
    opt->is_set = true;
    return true;
}

/**
 * @brief Parse and assign optarg to the matched option according to its type.
 * @return true on success, false on type mismatch or validation error.
 */
static bool assign_optarg(cli_option_internal * opt, const char * optarg) {
    switch (opt->def.type) {
        case CLI_TYPE_INTEGER: {
            return set_long_value(opt, optarg);
        }
        case CLI_TYPE_FLOAT: {
            return set_double_value(opt, optarg);
        }
        case CLI_TYPE_STRING: {
            return set_string_value(opt, optarg, true);
        }
        case CLI_TYPE_PATH: {
            if (!validate_path(optarg)) return false;
            return set_string_value(opt, optarg, true);
        }
    }
    return false;
}

/**
 * @brief Apply the default value to an unset optional option.
 */
static void apply_default(cli_option_internal *opt) {
    switch (opt->def.type) {
        case CLI_TYPE_INTEGER:
            opt->parsed_value.long_value = opt->def.default_value.long_value;
            break;
        case CLI_TYPE_FLOAT:
            opt->parsed_value.double_value = opt->def.default_value.double_value;
            break;
        case CLI_TYPE_STRING:
        /* fallthrough */
        case CLI_TYPE_PATH:
            // Points to the caller's string — not owned by us
            set_string_value(opt, opt->def.default_value.string_value, false);
            return;     // set_string_value already sets is_set to true, so we return early
    }
    opt->is_set = true;
}

/**
 * @brief Look up an option in the hashmap by name.
 * @return Pointer to the internal option, or NULL if not found.
 */
static cli_option_internal * lookup_option(const cli c, const char * name) {
    if (!c || !name) return NULL;
    cli_option_internal * opt = NULL;
    if (HashMap_peek(c->map, (HashMapKey) name, (HashMapValue *) &opt) == HASHMAP_OK) {
        return opt;
    }
    return NULL;
}

/**
 * @brief Build the getopt_long option tables from registered options.
 *
 * @param c              The CLI parser.
 * @param long_options   Output: caller-allocated array of size (option_count + 1).
 * @param short_options  Output: caller-allocated buffer of size (MAX_OPTIONS * 3 + 2).
 */
static void build_getopt_tables(const cli c, struct option * long_options, char * short_options) {
    int long_idx = 0;
    int short_idx = 0;

    short_options[short_idx++] = ':'; // return ':' for missing arguments

    for (size_t i = 0; i < c->option_count; i++) {
        const cli_option * def = &(c->options[i].def);

        if (def->option_name_long[0] != '\0') {
            long_options[long_idx].name     = def->option_name_long;
            long_options[long_idx].has_arg  = required_argument;
            long_options[long_idx].flag     = NULL;
            long_options[long_idx].val      = def->option_name_short[0] != '\0'
                                              ? (unsigned char)def->option_name_short[0]
                                              : 0;
            long_idx++;
        }

        if (def->option_name_short[0] != '\0') {
            short_options[short_idx++] = def->option_name_short[0];
            short_options[short_idx++] = ':';
        }
    }

    short_options[short_idx] = '\0';
    // Sentinel entry for long_options (already zeroed by calloc)
}

/*************************************************************************/
/*                          Public API                                   */
/*************************************************************************/

cli cli_create(void) {
    cli c = calloc(1, sizeof(cli_internal));
    if (!c) return NULL;

    c->map = HashMap_create(string_hash_fn, string_equals_fn);
    if (!c->map) {
        free(c);
        return NULL;
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
        HashMap_cleanup(c->map, NULL);
    }
    free(c);
}

bool cli_add_option(const cli c, cli_option o) {
    if (!c || c->option_count >= MAX_OPTIONS) return false;

    // Strip leading dashes — returns 0 for empty or all-dashes names
    strip_leading_dashes(o.option_name_short, MAX_SHORT_NAME);
    strip_leading_dashes(o.option_name_long, MAX_LONG_NAME);

    // At least one name must be non-empty
    if (o.option_name_short[0] == '\0' && o.option_name_long[0] == '\0') return false;

    cli_option_internal * opt = &c->options[c->option_count];
    opt->def = o;
    opt->is_set = false;
    opt->owns_string = false;

    // Register both name forms in the hashmap
    if (o.option_name_short[0] != '\0') {
        HashMap_put(c->map, (HashMapKey) opt->def.option_name_short, (HashMapValue) opt);
    }
    if (o.option_name_long[0] != '\0') {
        HashMap_put(c->map, (HashMapKey) opt->def.option_name_long, (HashMapValue) opt);
    }

    c->option_count++;
    return true;
}

bool cli_parse(const cli c, int argc, const char *argv[]) {
    if (!c || !argv || argc < 1) return false;

    // getopt_long may permute argv, so work on a mutable copy
    char **argv_copy = malloc(argc * sizeof(char *));
    if (!argv_copy) return false;
    for (int i = 0; i < argc; i++) {
        argv_copy[i] = strdup(argv[i]);
        if (!argv_copy[i]) {
            for (int j = 0; j < i; j++) free(argv_copy[j]);
            free(argv_copy);
            return false;
        }
    }

    struct option * long_options = calloc(c->option_count + 1, sizeof(struct option));
    if (!long_options) {
        for (int i = 0; i < argc; i++) free(argv_copy[i]);
        free(argv_copy);
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

    while ((opt_val = getopt_long(argc, argv_copy, short_options, long_options, &option_index)) != -1) {
        if (opt_val == '?') continue;   // Unknown option — ignore
        if (opt_val == ':') {           // Missing required argument
            success = false;
            break;
        }

        // Resolve the matched option via the hashmap
        cli_option_internal * found = NULL;
        if (opt_val == 0) {
            // Long-only option (no short alias): use name from long_options table
            found = lookup_option(c, long_options[option_index].name);
        } 
        else {
            char short_name[2] = {(char)opt_val, '\0'};
            found = lookup_option(c, short_name);
        }

        if (found && optarg && !assign_optarg(found, optarg)) {
            success = false;
        }
    }

    opterr = old_opterr;

    // Apply defaults for unset options; fail on missing required ones
    for (size_t i = 0; i < c->option_count; i++) {
        if (!c->options[i].is_set) {
            if (c->options[i].def.required) {
                success = false;
            } 
            else {
                apply_default(&c->options[i]);
            }
        }
    }

    free(long_options);
    for (int i = 0; i < argc; i++) free(argv_copy[i]);
    free(argv_copy);

    return success;
}

bool cli_get_integer(const cli c, const char * name, long * out_value) {
    cli_option_internal * opt = lookup_option(c, name);
    if (!opt || !opt->is_set || opt->def.type != CLI_TYPE_INTEGER) return false;
    if (out_value) *out_value = opt->parsed_value.long_value;
    return true;
}

bool cli_get_float(const cli c, const char * name, double * out_value) {
    cli_option_internal * opt = lookup_option(c, name);
    if (!opt || !opt->is_set || opt->def.type != CLI_TYPE_FLOAT) return false;
    if (out_value) *out_value = opt->parsed_value.double_value;
    return true;
}

bool cli_get_string(const cli c, const char * name, const char ** out_value) {
    cli_option_internal * opt = lookup_option(c, name);
    if (!opt || !opt->is_set) return false;
    if (opt->def.type != CLI_TYPE_STRING && opt->def.type != CLI_TYPE_PATH) return false;
    if (out_value) *out_value = opt->parsed_value.string_value;
    return true;
}

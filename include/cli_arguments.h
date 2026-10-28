#ifndef _CLI_ARGUMENTS_H_
#define _CLI_ARGUMENTS_H_

#include "cli.h"

#define PICTURE_FILE_EXTENSION ".bmp"

// CLI Option Name Constants
#define CLI_OPT_EMBED_LONG              "embed"
#define CLI_OPT_IN_LONG                 "in"
#define CLI_OPT_EXTRACT_LONG            "extract"
#define CLI_OPT_PICTURE_SHORT           "p"
#define CLI_OPT_PICTURE_LONG            "picture"
#define CLI_OPT_OUT_LONG                "out"
#define CLI_OPT_STEG_LONG               "steg"
#define CLI_OPT_CIPHER_ALG_SHORT        "a"
#define CLI_OPT_CIPHER_ALG_LONG         "cipher-algorithm"
#define CLI_OPT_CIPHER_MODE_SHORT       "m"
#define CLI_OPT_CIPHER_MODE_LONG        "cipher-mode"
#define CLI_OPT_PASS_LONG               "pass"
#define CLI_OPT_VERBOSE_SHORT           "v"
#define CLI_OPT_VERBOSE_LONG            "verbose"

static const char * STEGANOGRAPHY_CHOICES[] = {
    "LSB1",
    "LSB4",
    "LSBI",
    nullptr
};

static constexpr char DEFAULT_CIPHER[] = "AES128";
static const char * CIPHER_CHOICES[] = {
    DEFAULT_CIPHER,
    "AES192",
    "AES256",
    "3DES",
    nullptr
};

static constexpr char DEFAULT_MODE[] = "CBC";
static const char * MODE_CHOICES[] = {
    "ECB",
    "CFB",
    "OFB",
    DEFAULT_MODE,
    nullptr
};

static const cli_option COMMAND_LINE_OPTIONS[] = {
    
    // EMBEDDING-ONLY
    {
        .option_name_long = CLI_OPT_EMBED_LONG,
        .type = CLI_TYPE_FLAG
    },
    {
        .option_name_long = CLI_OPT_IN_LONG,
        .type = CLI_TYPE_PATH,
        .required = false
    },

    // EXTRACTION-ONLY
    {
        .option_name_long = CLI_OPT_EXTRACT_LONG,
        .type = CLI_TYPE_FLAG
    },

    // COMMON REQUIRED
    {
        .option_name_short = CLI_OPT_PICTURE_SHORT,
        .option_name_long = CLI_OPT_PICTURE_LONG,
        .type = CLI_TYPE_PATH,
        .required = true
    },
    {
        .option_name_long = CLI_OPT_OUT_LONG,
        .type = CLI_TYPE_PATH,
        .required = true
    },
    {
        .option_name_long = CLI_OPT_STEG_LONG,
        .type = CLI_TYPE_CHOICE,
        .required = true,
        .choices = STEGANOGRAPHY_CHOICES,
        .case_insensitive = true
    },

    // COMMON OPTIONAL
    {
        .option_name_short = CLI_OPT_CIPHER_ALG_SHORT,
        .option_name_long = CLI_OPT_CIPHER_ALG_LONG,
        .type = CLI_TYPE_CHOICE,
        .required = false,
        .default_value = {.string_value = DEFAULT_CIPHER},
        .choices = CIPHER_CHOICES,
        .case_insensitive = true,
    },
    {
        .option_name_short = CLI_OPT_CIPHER_MODE_SHORT,
        .option_name_long = CLI_OPT_CIPHER_MODE_LONG,
        .type = CLI_TYPE_CHOICE,
        .required = false,
        .default_value = {.string_value = DEFAULT_MODE},
        .choices = MODE_CHOICES,
        .case_insensitive = true
    },
    {
        .option_name_long = CLI_OPT_PASS_LONG,
        .type = CLI_TYPE_STRING,
        .required = false,
        .default_value = {.string_value = ""}
    },
    {
        .option_name_short = CLI_OPT_VERBOSE_SHORT,
        .option_name_long = CLI_OPT_VERBOSE_LONG,
        .type = CLI_TYPE_FLAG
    }

};

cli parse_command_line(int argc, const char * argv[]);
void print_config(cli c);
bool verbose(cli c);

#endif // _CLI_ARGUMENTS_H_

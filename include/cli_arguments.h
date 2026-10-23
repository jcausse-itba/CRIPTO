#ifndef _CLI_ARGUMENTS_H_
#define _CLI_ARGUMENTS_H_

#include "cli.h"

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
        .option_name_long = "embed",
        .type = CLI_TYPE_FLAG
    },
    {
        .option_name_long = "in",
        .type = CLI_TYPE_PATH,
        .required = false
    },

    // EXTRACTION-ONLY
    {
        .option_name_long = "extract",
        .type = CLI_TYPE_FLAG
    },

    // COMMON REQUIRED
    {
        .option_name_short = "p",
        .option_name_long = "picture",
        .type = CLI_TYPE_PATH,
        .required = true
    },
    {
        .option_name_long = "out",
        .type = CLI_TYPE_PATH,
        .required = true
    },
    {
        .option_name_long = "steg",
        .type = CLI_TYPE_CHOICE,
        .required = true,
        .choices = STEGANOGRAPHY_CHOICES,
        .case_insensitive = true
    },

    // COMMON OPTIONAL
    {
        .option_name_short = "a",
        .option_name_long = "cipher-algorithm",
        .type = CLI_TYPE_CHOICE,
        .required = false,
        .default_value = {.string_value = DEFAULT_CIPHER},
        .choices = CIPHER_CHOICES,
        .case_insensitive = true,
    },
    {
        .option_name_short = "m",
        .option_name_long = "cipher-mode",
        .type = CLI_TYPE_CHOICE,
        .required = false,
        .default_value = {.string_value = DEFAULT_MODE},
        .choices = MODE_CHOICES,
        .case_insensitive = true
    },
    {
        .option_name_long = "pass",
        .type = CLI_TYPE_STRING,
        .required = false,
        .default_value = {.string_value = ""}
    }

};

cli parse_command_line(int argc, const char * argv[]);

#endif // _CLI_ARGUMENTS_H_

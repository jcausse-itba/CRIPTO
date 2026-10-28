#include "cli_arguments.h"
#include "custom_assert.h"
#include <string.h>
#include <stdio.h>

#define ARRAY_LEN(arr) ((int)(sizeof(arr) / sizeof((arr)[0])))

static void test_embed_full_encryption_example_1() {
    // Enunciado 5.1 Ejemplo 1:
    // $stegobmp -embed -in "mensaje1.txt" -p "imagen1.bmp" -out "imagenmas1.bmp" -steg LSBI -a 3des -m cbc -pass "oculto"
    const char *argv[] = {
        "stegobmp",
        "-embed",
        "-in", "mensaje1.txt",
        "-p", "imagen1.bmp",
        "-out", "imagenmas1.bmp",
        "-steg", "LSBI",
        "-a", "3des",
        "-m", "cbc",
        "-pass", "oculto"
    };

    cli c = parse_command_line(ARRAY_LEN(argv), argv);
    ASSERT_MSG(c != nullptr, "Valid full embed invocation (Example 1) should succeed");
    if (!c) return;

    bool embed = false, extract = true;
    ASSERT(cli_get_flag(c, CLI_OPT_EMBED_LONG, &embed) && embed == true);
    ASSERT(cli_get_flag(c, CLI_OPT_EXTRACT_LONG, &extract) && extract == false);

    const char *in_val = nullptr, *p_val = nullptr, *out_val = nullptr;
    const char *steg_val = nullptr, *alg_val = nullptr, *mode_val = nullptr, *pass_val = nullptr;

    ASSERT(cli_get_string(c, CLI_OPT_IN_LONG, &in_val) && strcmp(in_val, "mensaje1.txt") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_PICTURE_SHORT, &p_val) && strcmp(p_val, "imagen1.bmp") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_OUT_LONG, &out_val) && strcmp(out_val, "imagenmas1.bmp") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_STEG_LONG, &steg_val) && strcmp(steg_val, "LSBI") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_CIPHER_ALG_SHORT, &alg_val) && strcmp(alg_val, "3DES") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_CIPHER_MODE_SHORT, &mode_val) && strcmp(mode_val, "CBC") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_PASS_LONG, &pass_val) && strcmp(pass_val, "oculto") == 0);

    cli_destroy(c);
}

static void test_embed_no_encryption_example_2() {
    // Enunciado 5.1 Ejemplo 2:
    // $stegobmp -embed -in "mensaje1.txt" -p "imagen1.bmp" -out "imagenmas1.bmp" -steg LSBI
    const char *argv[] = {
        "stegobmp",
        "-embed",
        "-in", "mensaje1.txt",
        "-p", "imagen1.bmp",
        "-out", "imagenmas1.bmp",
        "-steg", "LSBI"
    };

    cli c = parse_command_line(ARRAY_LEN(argv), argv);
    ASSERT_MSG(c != nullptr, "Valid unencrypted embed invocation (Example 2) should succeed");
    if (!c) return;

    bool embed = false, pass_set = true;
    ASSERT(cli_get_flag(c, CLI_OPT_EMBED_LONG, &embed) && embed == true);
    ASSERT(cli_is_set(c, CLI_OPT_PASS_LONG, &pass_set) && pass_set == false);

    const char *in_val = nullptr, *p_val = nullptr, *out_val = nullptr, *steg_val = nullptr;
    ASSERT(cli_get_string(c, CLI_OPT_IN_LONG, &in_val) && strcmp(in_val, "mensaje1.txt") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_PICTURE_LONG, &p_val) && strcmp(p_val, "imagen1.bmp") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_OUT_LONG, &out_val) && strcmp(out_val, "imagenmas1.bmp") == 0);
    ASSERT(cli_get_string(c, CLI_OPT_STEG_LONG, &steg_val) && strcmp(steg_val, "LSBI") == 0);

    cli_destroy(c);
}

static void test_embed_encryption_defaults_combinations() {
    // 1. Algorithm + password, no mode -> defaults to CBC
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "secret.txt", "-p", "carrier.bmp",
            "-out", "stego.bmp", "-steg", "LSB1", "-a", "aes256", "-pass", "my_pass"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c != nullptr, "Embed with -a and -pass (no -m) should succeed");
        if (c) {
            const char *alg = nullptr, *mode = nullptr, *pass = nullptr;
            bool mode_set = true;
            ASSERT(cli_get_string(c, CLI_OPT_CIPHER_ALG_LONG, &alg) && strcmp(alg, "AES256") == 0);
            ASSERT(cli_is_set(c, CLI_OPT_CIPHER_MODE_LONG, &mode_set) && mode_set == false);
            ASSERT(cli_get_string(c, CLI_OPT_CIPHER_MODE_LONG, &mode) && strcmp(mode, "CBC") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_PASS_LONG, &pass) && strcmp(pass, "my_pass") == 0);
            cli_destroy(c);
        }
    }

    // 2. Mode + password, no algorithm -> defaults to AES128
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "secret.txt", "-p", "carrier.bmp",
            "-out", "stego.bmp", "-steg", "LSB4", "-m", "ofb", "-pass", "my_pass"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c != nullptr, "Embed with -m and -pass (no -a) should succeed");
        if (c) {
            const char *alg = nullptr, *mode = nullptr, *pass = nullptr;
            bool alg_set = true;
            ASSERT(cli_is_set(c, CLI_OPT_CIPHER_ALG_LONG, &alg_set) && alg_set == false);
            ASSERT(cli_get_string(c, CLI_OPT_CIPHER_ALG_LONG, &alg) && strcmp(alg, "AES128") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_CIPHER_MODE_LONG, &mode) && strcmp(mode, "OFB") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_PASS_LONG, &pass) && strcmp(pass, "my_pass") == 0);
            cli_destroy(c);
        }
    }

    // 3. Only password -> defaults to AES128 + CBC
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "secret.txt", "-p", "carrier.bmp",
            "-out", "stego.bmp", "-steg", "lsb1", "-pass", "only_pass"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c != nullptr, "Embed with only -pass (no -a, no -m) should succeed");
        if (c) {
            const char *alg = nullptr, *mode = nullptr, *pass = nullptr, *steg = nullptr;
            ASSERT(cli_get_string(c, CLI_OPT_CIPHER_ALG_LONG, &alg) && strcmp(alg, "AES128") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_CIPHER_MODE_LONG, &mode) && strcmp(mode, "CBC") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_PASS_LONG, &pass) && strcmp(pass, "only_pass") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_STEG_LONG, &steg) && strcmp(steg, "LSB1") == 0);
            cli_destroy(c);
        }
    }
}

static void test_extract_valid_invocations() {
    // 1. Unencrypted extraction (Section 5.2)
    {
        const char *argv[] = {
            "stegobmp", "-extract", "-p", "portador.bmp", "-out", "extraido.txt", "-steg", "LSB4"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c != nullptr, "Valid unencrypted extract invocation should succeed");
        if (c) {
            bool extract = false, embed = true, in_set = true, pass_set = true;
            const char *p_val = nullptr, *out_val = nullptr, *steg_val = nullptr;
            ASSERT(cli_get_flag(c, CLI_OPT_EXTRACT_LONG, &extract) && extract == true);
            ASSERT(cli_get_flag(c, CLI_OPT_EMBED_LONG, &embed) && embed == false);
            ASSERT(cli_is_set(c, CLI_OPT_IN_LONG, &in_set) && in_set == false);
            ASSERT(cli_is_set(c, CLI_OPT_PASS_LONG, &pass_set) && pass_set == false);
            ASSERT(cli_get_string(c, CLI_OPT_PICTURE_SHORT, &p_val) && strcmp(p_val, "portador.bmp") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_OUT_LONG, &out_val) && strcmp(out_val, "extraido.txt") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_STEG_LONG, &steg_val) && strcmp(steg_val, "LSB4") == 0);
            cli_destroy(c);
        }
    }

    // 2. Encrypted extraction with reordered arguments and long option aliases
    {
        const char *argv[] = {
            "stegobmp",
            "--out", "recovered.png",
            "--cipher-mode", "cfb",
            "--pass", "dec_pass",
            "--picture", "stego_carrier.bmp",
            "-a", "aes192",
            "-steg", "lsbi",
            "-extract"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c != nullptr, "Valid encrypted extract with permuted/long options should succeed");
        if (c) {
            const char *p_val = nullptr, *out_val = nullptr, *steg_val = nullptr;
            const char *alg_val = nullptr, *mode_val = nullptr, *pass_val = nullptr;
            ASSERT(cli_get_string(c, CLI_OPT_PICTURE_LONG, &p_val) && strcmp(p_val, "stego_carrier.bmp") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_OUT_LONG, &out_val) && strcmp(out_val, "recovered.png") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_STEG_LONG, &steg_val) && strcmp(steg_val, "LSBI") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_CIPHER_ALG_LONG, &alg_val) && strcmp(alg_val, "AES192") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_CIPHER_MODE_LONG, &mode_val) && strcmp(mode_val, "CFB") == 0);
            ASSERT(cli_get_string(c, CLI_OPT_PASS_LONG, &pass_val) && strcmp(pass_val, "dec_pass") == 0);
            cli_destroy(c);
        }
    }
}

static void test_mode_and_in_parameter_mistakes() {
    // 1. Neither -embed nor -extract provided
    {
        const char *argv[] = {
            "stegobmp", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when neither -embed nor -extract is specified");
    }

    // 2. Both -embed and -extract provided simultaneously
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-extract", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when both -embed and -extract are specified");
    }

    // 3. -embed without -in
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when -embed is used without -in");
    }

    // 4. -extract with -in (forbidden during extraction)
    {
        const char *argv[] = {
            "stegobmp", "-extract", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when -extract is used with -in");
    }
}

static void test_encryption_without_password_mistakes() {
    // IMPORTANTE: No se puede encriptar/desencriptar sin password.
    // 1. -a provided without -pass (embed)
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1", "-a", "aes128"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when -a is specified without -pass in embed");
    }

    // 2. -m provided without -pass (embed)
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1", "-m", "ecb"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when -m is specified without -pass in embed");
    }

    // 3. Both -a and -m provided without -pass (extract)
    {
        const char *argv[] = {
            "stegobmp", "-extract", "-p", "img.bmp", "-out", "out.txt", "-steg", "LSBI", "-a", "3des", "-m", "cbc"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when -a and -m are specified without -pass in extract");
    }
}

static void test_missing_required_and_invalid_values_mistakes() {
    // 1. Missing -p (carrier bitmap)
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "msg.txt", "-out", "out.bmp", "-steg", "LSB1"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when required -p is missing");
    }

    // 2. Missing -out
    {
        const char *argv[] = {
            "stegobmp", "-extract", "-p", "img.bmp", "-steg", "LSB1"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when required -out is missing");
    }

    // 3. Missing -steg
    {
        const char *argv[] = {
            "stegobmp", "-extract", "-p", "img.bmp", "-out", "out.txt"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when required -steg is missing");
    }

    // 4. Invalid steganography algorithm choice (e.g. LSB2)
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB2"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail on invalid -steg choice 'LSB2'");
    }

    // 5. Invalid cipher algorithm choice (e.g. des)
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1", "-a", "des", "-pass", "secret"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail on invalid -a choice 'des'");
    }

    // 6. Invalid cipher mode choice (e.g. gcm)
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1", "-m", "gcm", "-pass", "secret"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail on invalid -m choice 'gcm'");
    }

    // 7. Option missing its argument value at end of argv
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1", "-pass"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail when -pass is passed without a value");
    }

    // 8. Unrecognized / misspelled option flag (e.g. -unknown or --password)
    {
        const char *argv[] = {
            "stegobmp", "-embed", "-in", "msg.txt", "-p", "img.bmp", "-out", "out.bmp", "-steg", "LSB1", "-unknown", "secret"
        };
        cli c = parse_command_line(ARRAY_LEN(argv), argv);
        ASSERT_MSG(c == nullptr, "Should fail on unrecognized option '-unknown'");
        if (c) cli_destroy(c);
    }
}

static void test_picture_bitmap_extension_validation() {
    // 1. Valid .bmp extensions (both -p and --picture, case-insensitivity, paths)
    {
        const char *valid_cases[] = {
            "image.bmp",
            "image.BMP",
            "image.Bmp",
            "/path/to/nested/carrier.bmp",
            "some.dir/another.dir/carrier.BMP",
            "archive.backup.bmp"
        };
        for (size_t i = 0; i < sizeof(valid_cases) / sizeof(valid_cases[0]); i++) {
            const char *argv[] = {
                "stegobmp", "-embed", "-in", "msg.txt", "-p", valid_cases[i],
                "-out", "out.bmp", "-steg", "LSB1"
            };
            cli c = parse_command_line(ARRAY_LEN(argv), argv);
            ASSERT_MSG(c != nullptr, "Valid bitmap picture extension should succeed");
            if (c) cli_destroy(c);
        }
    }

    // 2. Invalid picture extensions (should fail validation)
    {
        const char *invalid_cases[] = {
            "image.png",
            "image.jpg",
            "image.jpeg",
            "image.txt",
            "image.pdf",
            "image.gif",
            "carrier_no_ext",
            "/path/to/carrier_no_ext",
            "dir.bmp/carrier_no_ext",
            "carrier.bmp.txt",
            "carrier.bmp.bak",
            "carrier."
        };
        for (size_t i = 0; i < sizeof(invalid_cases) / sizeof(invalid_cases[0]); i++) {
            // Test with short option -p
            {
                const char *argv[] = {
                    "stegobmp", "-embed", "-in", "msg.txt", "-p", invalid_cases[i],
                    "-out", "out.bmp", "-steg", "LSB1"
                };
                cli c = parse_command_line(ARRAY_LEN(argv), argv);
                ASSERT_MSG(c == nullptr, "Picture without .bmp extension should fail validation");
                if (c) cli_destroy(c);
            }
            // Test with long option --picture in extract mode as well
            {
                const char *argv[] = {
                    "stegobmp", "-extract", "--picture", invalid_cases[i],
                    "-out", "out.txt", "-steg", "LSB1"
                };
                cli c = parse_command_line(ARRAY_LEN(argv), argv);
                ASSERT_MSG(c == nullptr, "Picture without .bmp extension should fail validation in extract mode");
                if (c) cli_destroy(c);
            }
        }
    }
}

int main() {
    // Redirect stderr to /dev/null during expected-failure tests to keep test output clean
    FILE *orig_stderr = stderr;
    FILE *null_out = fopen("/dev/null", "w");
    if (null_out) {
        stderr = null_out;
    }

    test_embed_full_encryption_example_1();
    test_embed_no_encryption_example_2();
    test_embed_encryption_defaults_combinations();
    test_extract_valid_invocations();
    test_mode_and_in_parameter_mistakes();
    test_encryption_without_password_mistakes();
    test_missing_required_and_invalid_values_mistakes();
    test_picture_bitmap_extension_validation();

    if (null_out) {
        stderr = orig_stderr;
        fclose(null_out);
    }

    return ASSERT_REPORT();
}

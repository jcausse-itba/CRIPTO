#include "string_utils.h"
#include "custom_assert.h"
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

static void test_str_equals_case_sensitive() {
    ASSERT(str_equals("hello", "hello", false));
    ASSERT(!str_equals("hello", "Hello", false));
    ASSERT(!str_equals("Hello", "hello", false));
    ASSERT(!str_equals("HELLO", "hello", false));
    ASSERT(!str_equals("hello", "world", false));
    ASSERT(!str_equals("abc", "abcdef", false));
    ASSERT(!str_equals("abcdef", "abc", false));
    ASSERT(str_equals("", "", false));
    ASSERT(!str_equals("", "a", false));
    ASSERT(!str_equals("a", "", false));
    ASSERT(str_equals("stegobmp 2026!", "stegobmp 2026!", false));
    ASSERT(!str_equals("stegobmp 2026!", "Stegobmp 2026!", false));
}

static void test_str_equals_case_insensitive() {
    ASSERT(str_equals("hello", "hello", true));
    ASSERT(str_equals("hello", "Hello", true));
    ASSERT(str_equals("Hello", "hello", true));
    ASSERT(str_equals("HELLO", "hello", true));
    ASSERT(str_equals("hElLo", "HeLlO", true));
    ASSERT(!str_equals("hello", "world", true));
    ASSERT(!str_equals("abc", "abcdef", true));
    ASSERT(!str_equals("abcdef", "abc", true));
    ASSERT(str_equals("", "", true));
    ASSERT(!str_equals("", "a", true));
    ASSERT(!str_equals("a", "", true));
    ASSERT(str_equals("StegoBMP-2026!", "stegobmp-2026!", true));
    ASSERT(!str_equals("StegoBMP-2026!", "stegobmp-2027!", true));
}

static void test_str_equals_null_and_pointer_cases() {
    const char *s = "pointer_test";
    ASSERT(str_equals(s, s, false));
    ASSERT(str_equals(s, s, true));

    ASSERT(str_equals(nullptr, nullptr, false));
    ASSERT(str_equals(nullptr, nullptr, true));

    ASSERT(!str_equals(nullptr, "test", false));
    ASSERT(!str_equals(nullptr, "test", true));
    ASSERT(!str_equals("test", nullptr, false));
    ASSERT(!str_equals("test", nullptr, true));
}

static void test_get_file_extension_basic() {
    char ext[32];

    ASSERT(get_file_extension("image.bmp", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".bmp") == 0);

    ASSERT(get_file_extension("document.pdf", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".pdf") == 0);

    ASSERT(get_file_extension("photo.png", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".png") == 0);

    ASSERT(get_file_extension("page.html", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".html") == 0);
}

static void test_get_file_extension_case_handling() {
    char ext[32];

    // Uppercase preserved when lowercase == false
    ASSERT(get_file_extension("image.BMP", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".BMP") == 0);

    // Uppercase converted to lowercase when lowercase == true
    ASSERT(get_file_extension("image.BMP", true, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".bmp") == 0);

    ASSERT(get_file_extension("file.TXT", true, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".txt") == 0);

    ASSERT(get_file_extension("file.TXT", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".TXT") == 0);

    ASSERT(get_file_extension("mixed.Tar.Gz", true, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".gz") == 0);

    ASSERT(get_file_extension("mixed.Tar.Gz", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".Gz") == 0);
}

static void test_get_file_extension_multiple_dots() {
    char ext[32];

    // Example directly from documentation: "my_spectacular_file.pdf.txt" -> ".txt"
    ASSERT(get_file_extension("my_spectacular_file.pdf.txt", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".txt") == 0);

    ASSERT(get_file_extension("my_spectacular_file.pdf.txt", true, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".txt") == 0);

    ASSERT(get_file_extension("archive.tar.gz", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".gz") == 0);

    ASSERT(get_file_extension("archive.tar.bz2", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".bz2") == 0);

    ASSERT(get_file_extension("dotted.name.with.many.dots.JPG", true, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".jpg") == 0);

    ASSERT(get_file_extension("double..dot.ext", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".ext") == 0);
}

static void test_get_file_extension_paths() {
    char ext[32];

    // Absolute Unix path
    ASSERT(get_file_extension("/home/user/pictures/cat.bmp", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".bmp") == 0);

    // Relative Unix path
    ASSERT(get_file_extension("./subdir/subsubdir/file.txt", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".txt") == 0);

    // Windows style path
    ASSERT(get_file_extension("C:\\Users\\user\\Documents\\report.PDF", true, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".pdf") == 0);

    // Mixed slashes
    ASSERT(get_file_extension("folder/sub\\file.png", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".png") == 0);

    // Dot in directory name, file has extension
    ASSERT(get_file_extension("/var/www/v1.0/index.php", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".php") == 0);

    ASSERT(get_file_extension("some.dir/another.dir/payload.bin", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".bin") == 0);

    // Dot in directory name, but file has NO extension -> returns false and writes null byte
    ASSERT(get_file_extension("/path.with.dots/binary_executable", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    ASSERT(get_file_extension("dir.v1\\binary", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);
}

static void test_get_file_extension_no_extension() {
    char ext[32];

    // Ensure output buffer gets cleared to null-terminating byte on failure
    strcpy(ext, "initial_value");
    ASSERT(get_file_extension("README", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    strcpy(ext, "initial_value");
    ASSERT(get_file_extension("Makefile", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    strcpy(ext, "initial_value");
    ASSERT(get_file_extension("/usr/bin/gcc", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    strcpy(ext, "initial_value");
    ASSERT(get_file_extension("", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    strcpy(ext, "initial_value");
    ASSERT(get_file_extension("/trailing/slash/", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    // Directory navigation references have no extension
    strcpy(ext, "initial_value");
    ASSERT(get_file_extension(".", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    strcpy(ext, "initial_value");
    ASSERT(get_file_extension("..", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    strcpy(ext, "initial_value");
    ASSERT(get_file_extension("/path/to/.", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    strcpy(ext, "initial_value");
    ASSERT(get_file_extension("/path/to/..", false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);
}

static void test_get_file_extension_dot_edge_cases() {
    char ext[32];

    // Trailing dot
    ASSERT(get_file_extension("file.", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".") == 0);

    ASSERT(get_file_extension("archive..", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".") == 0);

    // File with leading dot
    ASSERT(get_file_extension(".gitignore", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".gitignore") == 0);

    ASSERT(get_file_extension(".GITIGNORE", true, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".gitignore") == 0);

    // Hidden file with extension
    ASSERT(get_file_extension(".config.json", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".json") == 0);

    ASSERT(get_file_extension("/home/user/.bashrc", false, ext, sizeof(ext)) == true);
    ASSERT(strcmp(ext, ".bashrc") == 0);
}

static void test_get_file_extension_buffer_limits() {
    // Exact size needed for ".txt" is 5 bytes including '\0'
    char buf5[5];
    ASSERT(get_file_extension("file.txt", false, buf5, sizeof(buf5)) == true);
    ASSERT(strcmp(buf5, ".txt") == 0);

    // Truncation when buffer is smaller than extension
    char buf4[4];
    ASSERT(get_file_extension("file.txt", false, buf4, sizeof(buf4)) == true);
    ASSERT(strcmp(buf4, ".tx") == 0);

    char buf3[3];
    ASSERT(get_file_extension("file.txt", false, buf3, sizeof(buf3)) == true);
    ASSERT(strcmp(buf3, ".t") == 0);

    char buf2[2];
    ASSERT(get_file_extension("file.txt", false, buf2, sizeof(buf2)) == true);
    ASSERT(strcmp(buf2, ".") == 0);

    char buf1[1];
    ASSERT(get_file_extension("file.txt", false, buf1, sizeof(buf1)) == true);
    ASSERT(strcmp(buf1, "") == 0);

    // Canary check to ensure no out-of-bounds writes
    char canary_buf[8] = {'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A'};
    ASSERT(get_file_extension("file.txt", false, canary_buf, 4) == true);
    // buf[0..2] should be ".tx", buf[3] should be '\0', and canary_buf[4..7] should be 'A'
    ASSERT(strcmp(canary_buf, ".tx") == 0);
    ASSERT(canary_buf[4] == 'A');
    ASSERT(canary_buf[5] == 'A');
    ASSERT(canary_buf[6] == 'A');
    ASSERT(canary_buf[7] == 'A');
}

static void test_get_file_extension_null_and_invalid_params() {
    char ext[32] = "previous_value";

    // NULL filename -> returns false, writes null-terminating byte
    ASSERT(get_file_extension(nullptr, false, ext, sizeof(ext)) == false);
    ASSERT(strcmp(ext, "") == 0);

    // NULL out_extension -> returns false, does not crash
    ASSERT(get_file_extension("file.txt", false, nullptr, 32) == false);

    // max_size = 0 -> returns false, does not write to buffer, does not crash
    char untouched[8] = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'};
    ASSERT(get_file_extension("file.txt", false, untouched, 0) == false);
    for (size_t i = 0; i < sizeof(untouched); i++) {
        ASSERT(untouched[i] == 'X');
    }

    // Both NULL / 0 -> returns false
    ASSERT(get_file_extension(nullptr, false, nullptr, 0) == false);
}

int main() {
    test_str_equals_case_sensitive();
    test_str_equals_case_insensitive();
    test_str_equals_null_and_pointer_cases();

    test_get_file_extension_basic();
    test_get_file_extension_case_handling();
    test_get_file_extension_multiple_dots();
    test_get_file_extension_paths();
    test_get_file_extension_no_extension();
    test_get_file_extension_dot_edge_cases();
    test_get_file_extension_buffer_limits();
    test_get_file_extension_null_and_invalid_params();

    return ASSERT_REPORT();
}

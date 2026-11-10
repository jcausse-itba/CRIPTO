# AGENT_MEMORY.md - StegoBMP Context & Knowledge Base

> **Notice for Future AI Agents:**  
> This file is the primary context and persistent memory handoff for the `CRIPTO` (`stegobmp`) project. Read this file completely before making architectural decisions or modifying code. It documents the domain rules, past discussions, architectural choices, consensus reached with the user (Juan Ignacio / `juani`), solved bugs, and the roadmap for upcoming tasks.

---

## 1. Project Overview & Meta-Information

- **University Course:** 72.04 Criptografía y Seguridad (ITBA - Instituto Tecnológico de Buenos Aires).
- **Group:** Grupo 10.
- **Application Name:** `stegobmp` (CLI executable).
- **Primary Goal:** Hide (embed) and recover (extract) secret files inside 24 bpp uncompressed Windows Bitmap (BMP V3) carrier images using steganographic algorithms (`LSB1`, `LSB4`, `LSBI`), with optional symmetric cryptography (OpenSSL).
- **Delivery Deadline:** November 16.
- **Target Platform:** Linux / WSL2 (Ubuntu 22.04+ LTS / ITBA "Pampero" server environment).
- **Language Standard:** ISO C23 (`-std=c23`).
- **Compiler Flags:** Strict `-Wall -Wextra -Werror -pedantic`.
- **Sanitizers:** AddressSanitizer and UndefinedBehaviorSanitizer enabled by default (`-fsanitize=address,undefined`). All builds and tests must pass with 0 warnings and 0 sanitizer errors.

### Communication & Code Style Rules
1. **User Communication:** Spanish (native language of the user).
2. **Code & Comments:** English across all headers (`include/`), source files (`src/`), and tests (`test/`).
3. **Pointers & Nulls:** Use modern C23 `nullptr` (not `NULL` or `0`).
4. **Markdown Formatting:** Always provide clickable links using the `file://` scheme (e.g. `[bitmap.c](file:///home/juani/CRIPTO/src/bitmap.c#L10-L25)`).

---

## 2. Directory Structure & Key Files

```text
/home/juani/CRIPTO/
├── CMakeLists.txt                # Main build definitions (stego_core, stegobmp, CTest targets)
├── Makefile                      # CLI convenience wrappers (dev, release, test, clean, inspect)
├── README.md                     # User documentation with execution examples and parameter guide
├── doc/
│   ├── Enunciado.pdf             # Official assignment requirements and specifications
│   ├── LSB-Improved.pdf          # Research paper: Majeed & Sulaiman (2015) - LSBI reference
│   ├── LSBI_Summary.md           # Condensed mathematical & algorithmic summary of the LSBI paper
│   └── AGENT_MEMORY.md           # THIS FILE (Persistent memory for AI agents)
├── external-utilities/
│   ├── bmp_inspector.py          # Python utility to inspect BMP headers and validate V3 compatibility
│   ├── modify_bmp_pixel.py       # Helper script to corrupt/modify random pixels for robustness checks
│   └── openssl_test.c            # Standalone OpenSSL proof-of-concept testing cryptographic primitives
├── images/
│   ├── cat_v3.bmp                # 700x700 24bpp BMP V3 test carrier (stride = 2100, padding = 0)
│   └── eiffel_tower_v3.bmp       # 800x800 24bpp BMP V3 test carrier (stride = 2400, padding = 0)
├── include/
│   ├── bitmap_v3.h               # Microsoft BMP V3 packed struct definitions (BITMAPFILEHEADER, etc.)
│   ├── bitmap.h                  # Bitmap carrier abstraction API
│   ├── cli_arguments.h           # Stegobmp CLI option definitions and X-Macro choice lists
│   ├── cryptography.h            # Symmetric cipher & key derivation interface
│   ├── embed.h                   # Task handler for embedding payload into carrier
│   ├── extract.h                 # Task handler for extracting payload from carrier
│   ├── io_utils.h                # File existence and size helpers
│   └── steganography.h           # Steganography strategy dispatch table and public API
├── lib/
│   ├── cli.h / cli.c             # Generalized command-line argument parser (getopt_long_only wrapper)
│   ├── hashmap.h / hashmap.c     # Hash map data structure (used internally by CLI parser)
│   ├── logger.h / logger.c       # ANSI-colored logging utility (LOG_INFO, LOG_WARN, LOG_ERROR)
│   └── string_utils.h / string_utils.c # String comparison (case-insensitive) & extension parsing
├── src/
│   ├── bitmap.c                  # In-memory BMP loader, byte iterator, and saver
│   ├── cli_arguments.c           # Stegobmp argument registration, validation, and defaults
│   ├── cryptography.c           # OpenSSL EVP encryption/decryption, PBKDF2 key derivation
│   ├── embed.c                   # Embed workflow: payload bundling, encryption, and carrier save
│   ├── extract.c                 # Extract workflow: payload extraction, decryption, and disk save
│   ├── io_utils.c                # I/O validation implementations
│   ├── main.c                   # CLI entrypoint (logger initialization, dispatch to embed/extract)
│   └── steganography.c           # Strategies for LSB1, LSB4, and LSBI
└── test/
    ├── custom_assert.h           # Lightweight test framework reporting passed/failed assertions
    ├── bitmap_test.c             # Unit tests for BMP loading, stride, padding, and byte iteration
    ├── cli_arguments_test.c      # Unit tests for CLI parsing, option constraints, and defaults
    ├── cli_test.c                # Unit tests for the generalized CLI parser library
    ├── cryptography_test.c       # Black-box tests for AES/3DES in CBC/CFB/OFB/ECB and PBKDF2
    ├── hashmap_test.c            # Unit tests for the hashmap library
    ├── steganography_test.c      # Unit tests for LSB1, LSB4, LSBI inversion, and round-trips
    └── string_utils_test.c       # Unit tests for string utilities and extension extraction
```

---

## 3. Key Architectural Decisions & Consensus Reached

### 3.1. Bitmap Carrier Representation (`include/bitmap.h`, `src/bitmap.c`)
- **Complete In-Memory Dump:** The entire BMP file (headers, pixel data, and padding) is read into a single contiguous `malloc`'d memory buffer (`uint8_t * data`).
  - *Rationale:* LSBI is a two-pass algorithm (counting bit flips, then selective inversion). Stream-based file manipulation required complex synchronization and rewinding. Having the entire carrier in memory allows $O(1)$ byte iteration, in-place modification, and saving via a single `fwrite` in `bitmap_save`.
- **BMP Stride and Row Padding (Enunciado Section 5.4):**
  - Standard BMP aligns rows to 4-byte boundaries: `stride = ((row_bytes + 3) / 4) * 4`.
  - The specification (Section 5.4 of `doc/Enunciado.pdf`) states:
    > *"En algunos archivos bmp, se completa con 'padding' para lograr el tamaño de múltiplo de 4 bytes en las filas de píxeles. Para el esteganografiado usen todo el bloque completo de los pixeles + bytes de padding."*
  - **Decision:** Row padding bytes **ARE included** as carrier bytes in the sequential byte stream. `total_bytes = stride * height`. `bitmap_next_byte` and `bitmap_get_byte` advance continuously through `data_offset + row * stride + col` without skipping padding.

### 3.2. Steganography Payload Wire Format (Sections 5.3 & 5.4 of `Enunciado.pdf`)
There are two distinct wire formats depending on whether encryption is enabled:

1. **Unencrypted Wire Format:**
   ```text
   +------------------------------------+-----------------------+---------------------+
   | 4-byte Big-Endian real file size   | File Data Bytes       | File Extension      |
   | uint32_t (e.g. 0x00000005)         | (e.g. "HELLO")        | (e.g. ".txt\0")     |
   +------------------------------------+-----------------------+---------------------+
   ```
   - Total unencrypted payload bytes = `4 + file_size + strlen(ext) + 1`.
   - File extension **must begin with a dot (`'.'`) and terminate with a null byte (`'\0'`)**.

2. **Encrypted Wire Format:**
   - First, the unencrypted sequence (`4-byte Big-Endian size || file data || .ext\0`) is encrypted with the selected cipher, mode, and password. PKCS#5/PKCS#7 padding is added by OpenSSL if block size requires it.
   - Then, the final stego payload is assembled as:
   ```text
   +------------------------------------+-----------------------------------------------+
   | 4-byte Big-Endian ciphertext size  | Ciphertext Bytes (from encrypted plaintext)   |
   | uint32_t                           | (includes encrypted size, data, ext, padding) |
   +------------------------------------+-----------------------------------------------+
   ```
   - Total stego payload bytes = `4 + ciphertext_len`.
   - On extraction: read 4-byte ciphertext size, extract `ciphertext_len` bytes, decrypt with password, then parse the decrypted plaintext (`real_size || real_data || .ext\0`).

- **Big-Endian Encoding:** Portable ISO C shifts (`write_be32` and `read_be32`) are used instead of non-standard POSIX `htonl`/`ntohl`.

### 3.3. Command Line Interface & Encryption Defaults (`src/cli_arguments.c`)
- Uses single-dash or double-dash options via `getopt_long_only`:
  - Required: `-p <bmp>` (carrier), `-out <file>` (output), `-steg <LSB1|LSB4|LSBI>`.
  - Embed-only required: `-embed`, `-in <file>`.
  - Extract-only: `-extract`.
  - Optional encryption: `-pass <password>`, `-a <aes128|aes192|aes256|3des>`, `-m <ecb|cfb|ofb|cbc>`.
- **Default Resolution Rules (Enunciado Section 5.1/5.2):**
  - If `-pass` is absent $\rightarrow$ No encryption/decryption (steganography only).
  - If `-pass` is present alone $\rightarrow$ Defaults to `AES128` in `CBC` mode.
  - If `-a <alg>` + `-pass` (no `-m`) $\rightarrow$ Mode defaults to `CBC`.
  - If `-m <mode>` + `-pass` (no `-a`) $\rightarrow$ Algorithm defaults to `AES128`.
  - If `-a` or `-m` are passed without `-pass` $\rightarrow$ Parsing fails with an explicit error.
- **X-Macros:** Defined in `include/cli_arguments.h` for cipher algorithms and modes, generating static string choice arrays without duplication.

### 3.4. Cryptography Module (`include/cryptography.h`, `src/cryptography.c`)
- **Key & IV Derivation:** OpenSSL `PKCS5_PBKDF2_HMAC`:
  - Hash algorithm: `SHA-256`.
  - Iterations: `10000`.
  - Salt: Fixed 8-byte zero salt `0x0000000000000000` (`const uint8_t FIXED_SALT[8] = {0}`).
  - Extracts both `key` and `iv` from the derived key material buffer (`key_len + iv_len`).
- **Ciphers & Modes Supported:**
  - `aes128`, `aes192`, `aes256` in `ecb`, `cfb` (CFB8), `ofb` (128-bit), `cbc`.
  - `3des` in `ecb`, `cfb` (CFB8), `ofb` (128-bit), `cbc` (maps to OpenSSL `EVP_des_ede3_*`).
  - ECB mode uses no IV (`iv_len = 0`).
- **Padding:** OpenSSL PKCS#5/PKCS#7 padding is left enabled for block modes.

### 3.5. Steganography Algorithms (`src/steganography.c`)

#### LSB1
- Modifies bit 0 (the least significant bit) of every carrier byte.
- Bit order: Most Significant Bit of each payload byte first (bit 7 down to bit 0).
- Capacity: `carrier_bytes / 8`.

#### LSB4
- Modifies the 4 least significant bits of every carrier byte.
- Nibble order: High nibble first (bits 7..4 in carrier byte 0), Low nibble second (bits 3..0 in carrier byte 1).
- Capacity: `carrier_bytes / 2`.

#### LSBI (LSB Improved - Majeed & Sulaiman, 2015)
- **Control Bits in First 4 Bytes:**
  - The first 4 carrier bytes (indices 0, 1, 2, 3: B0, G0, R0, B1) store the 4 inversion flags for patterns `'00'`, `'01'`, `'10'`, `'11'` using normal LSB1. All 3 channels are used for these 4 bytes, and they are never inverted.
  - Bit value `1` means the pattern was inverted; `0` means it was not.
- **Selective Channel Usage for Secret Payload (Crucial User-Confirmed Decision):**
  - Starting at carrier byte index 4 (`G1`), **the Red channel is skipped** (`(carrier_idx++) % 3 == 2`).
  - Secret payload bits are embedded/extracted **only in Blue and Green channels**.
- **Pattern Definition:**
  - Pattern of a carrier byte is defined by its 2nd and 3rd LSBs (bits 1 and 2): `pattern = (byte >> 1) & 0x03U`.
- **Pass 1 (Evaluation):**
  - Candidate embedding replaces LSB with the secret bit.
  - Counts `changed[pattern]` vs `unchanged[pattern]`.
- **Inversion Criteria:**
  - If `changed[pattern] > unchanged[pattern]`, `invert[pattern] = true`, and the corresponding control bit in header bytes 0..3 is set to `1`.
- **Pass 2 (Inversion):**
  - For every payload carrier byte whose pattern has `invert[pattern] == true`, the LSB is inverted (`*carrier ^= 0x01U`).
- **Incremental Extraction (`lsbi_extract_bytes`):**
  - Uses `bitmap_get_cursor(bmp)`. When `cursor == 0`, it extracts the 4 pattern bits from header bytes [0..3] and caches them in a static array.
  - Chunked sequential extraction calls (size, payload, extension) proceed seamlessly without desynchronization.
- **Capacity:**
  - `rem = carrier_bytes - 4`.
  - `skipped_red = (rem + 1) / 3`.
  - `usable_bits = rem - skipped_red`.
  - `max_capacity = usable_bits / 8`.

---

## 4. Past Bugs Solved & Gotchas to Avoid

1. **`fileno` implicit declaration under `-std=c23 -pedantic` (`lib/logger.c`):**
   - In ISO C23, `fileno()` is a POSIX function, not ISO C.
   - **Fix:** Define `#define _POSIX_C_SOURCE 200809L` at the very top of `lib/logger.c` before `<stdio.h>`.
2. **String initialization with trailing null in array of exact length (`test/cryptography_test.c`):**
   - GCC 15 emits `-Werror=unterminated-string-initialization` on `const uint8_t data[16] = "0123456789abcdef";` because of the missing null terminator.
   - **Fix:** Use unsized array `const uint8_t data[] = "0123456789abcdef";`.
3. **`inline` functions without body in header:**
   - Defining `inline __attribute__((always_inline))` in public headers without a static or inline body causes GCC linker errors. Keep internal functions `static` in `.c` files and export standard function declarations in `.h`.
4. **Renaming `bitmap_reader` $\rightarrow$ `bitmap`:**
   - Originally the module was called `bitmap_reader`, but since it handles loading, reading, in-place writing, and saving, the user requested removing "reader". Keep all types as `bitmap` (`include/bitmap.h`).
5. **Big-Endian payload size representation:**
   - Intel/x86_64 machines are Little-Endian. Always serialize/deserialize 32-bit header integers using `write_be32` and `read_be32`.

---

## 5. Build & Test Commands

Always verify from the workspace root (`/home/juani/CRIPTO`):

```bash
# Build development target with ASan & UBSan
make dev

# Build optimized release target (no ASan)
make release

# Run all automated tests via CTest
make test

# Manual end-to-end embedding and extraction test
./stegobmp -embed -in README.md -p images/cat_v3.bmp -out /tmp/cat_test.bmp -steg LSBI -a 3des -m cbc -pass "secreto123" -v
./stegobmp -extract -p /tmp/cat_test.bmp -out /tmp/recovered -steg LSBI -a 3des -m cbc -pass "secreto123" -v
cmp README.md /tmp/recovered.md && echo "MATCH!"
```

---

## 6. Current State & Remaining Work ("Última Tarea")

### Completed (100% Passing):
- [x] BMP Carrier module with row-padding inclusion (`include/bitmap.h`, `src/bitmap.c`).
- [x] Command-line parser with validation, mutual exclusion, and default cipher handling (`src/cli_arguments.c`).
- [x] Cryptography module supporting AES-128/192/256 and 3DES in ECB/CFB/OFB/CBC via OpenSSL with PBKDF2 (`src/cryptography.c`).
- [x] Steganography strategy pattern supporting `LSB1`, `LSB4`, and `LSBI` with bit-inversion and selective Red channel skip (`src/steganography.c`).
- [x] Complete `embed` and `extract` workflows with Big-Endian size serialization, extension bundling, and OpenSSL encryption/decryption (`src/embed.c`, `src/extract.c`).
- [x] 7 test suites passing under ASan/UBSan (82 bitmap tests, 92 cli_arguments tests, 84 cli tests, 235 cryptography tests, 2398 hashmap tests, 130 steganography tests, 140 string_utils tests).

### Pending Deliverables (Where the next agent should focus):
1. **Steganalysis (Section 6 of `doc/Enunciado.pdf`):**
   - The teaching staff provides 4 mystery files (`.bmp` carriers with hidden content). One is hidden with `LSB1`, one with `LSB4`, one with `LSBI`, and a fourth with an unknown/alternative technique.
   - Steganalysis analysis tools, scripts, or detection logic (e.g., Chi-square test, RS analysis, sample pair analysis, or visual inspections via hex diffs/Python scripts).
2. **Written Report & Questions (Section 7 of `doc/Enunciado.pdf`):**
   - Ten specific questions (I to X) must be answered in a written report:
     - I. Discussion of the Majeed & Sulaiman paper (formal organization, algorithm description, notation ambiguities/errors).
     - II. Comparative analysis of LSB1, LSB4, and LSBI (PSNR, embedding impact, pros/cons table).
     - III. Detailed explanation of how each mystery carrier was analyzed and what was uncovered.
     - IV. Recursive/nested messages discovered in carriers.
     - V. Movie clip carrier identification.
     - VI. The 4th unknown steganography method identification and effectiveness analysis.
     - VII. Why LSBI is an improvement over standard LSB.
     - VIII. Alternative locations/methods for storing the 4-bit inversion pattern map.
     - IX. Difficulties encountered implementing the paper's algorithm.
     - X. Proposed improvements and future extensions for `stegobmp`.

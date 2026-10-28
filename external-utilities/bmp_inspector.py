import sys
import os
import ctypes
from pathlib import Path
from typing import Optional, Tuple


BASE_PATH = Path.cwd() / 'images'
HEADER_SIZE_BYTES = 54
TARGET_SIGNATURE = 0x4D42  # 0x4D42 = 'BM'

# Windows type aliases mapped to ctypes (all Little-Endian)
WORD = ctypes.c_uint16
DWORD = ctypes.c_uint32
LONG = ctypes.c_int32


class FileHeader(ctypes.LittleEndianStructure):
    _pack_ = 1                      # Disable compiler alignment padding
    _layout_ = 'ms'
    _fields_ = [
        ("file_type", WORD),        # 0x4D42 ('BM')
        ("file_size", DWORD),       # Size of file in bytes
        ("reserved1", WORD),        # 0
        ("reserved2", WORD),        # 0
        ("bitmap_offset", DWORD),   # Offset to pixel array
    ]


class BitmapV3InfoHeader(ctypes.LittleEndianStructure):
    _pack_ = 1                      # Disable compiler alignment padding
    _layout_ = 'ms'
    _fields_ = [
        ("size", DWORD),
        ("width", LONG),
        ("height", LONG),
        ("planes", WORD),
        ("bits_per_pixel", WORD),
        ("compression", DWORD),
        ("size_of_bitmap", DWORD),
        ("horz_resolution", LONG),
        ("vert_resolution", LONG),
        ("colors_used", DWORD),
        ("colors_important", DWORD),
    ]


def select_bmp(base_path: Path) -> Optional[Path]:
    try:
        bitmaps = [f for f in os.listdir(base_path) if f.lower().endswith('.bmp')]
    except (FileNotFoundError, PermissionError) as e:
        print(f"Error accessing directory: {e}", file=sys.stderr)
        return None

    if not bitmaps:
        print(f"No .bmp files found in {base_path}")
        return None

    print('Select bitmap:')
    for i, filename in enumerate(bitmaps):
        print(f"{i + 1:2d} --> {filename}")

    try:
        n = int(input(">>> "))
        if 1 <= n <= len(bitmaps):
            return Path(bitmaps[n - 1])
        print('Invalid selection')
        return None
    except ValueError:
        print('Invalid input')
        return None


def read_binary(file_path: Path, max_bytes: int | None = None) -> Optional[Tuple[FileHeader, BitmapV3InfoHeader]]:
    try:
        with open(file_path, 'rb') as f:
            file_header = FileHeader()
            f.readinto(file_header)
            info_header = BitmapV3InfoHeader()
            f.readinto(info_header)
            return file_header, info_header
    except (FileNotFoundError, PermissionError):
        return None


def main() -> None:
    filename = select_bmp(BASE_PATH)
    if not filename:
        return

    full_path = BASE_PATH / filename
    print(f'\nSelected file: {full_path}\n')

    result = read_binary(full_path, HEADER_SIZE_BYTES)
    if not result:
        print(f'Error: could not open file "{full_path}"', file=sys.stderr)
        return

    file_header, info_header = result

    # --- File Header (14 bytes) ---
    magic_ascii = file_header.file_type.to_bytes(2, 'little').decode('latin1', errors='replace')
    is_valid_sig = file_header.file_type == TARGET_SIGNATURE
    sig_status = "OK" if is_valid_sig else "ERROR (Expected 'BM' / 0x4d42)"

    print("=" * 45)
    print("           BMP FILE HEADER (14 bytes)        ")
    print("=" * 45)
    print(f"  file_type        : {file_header.file_type:#06x} ('{magic_ascii}') [{sig_status}]")
    print(f"  file_size        : {file_header.file_size:,} bytes")
    print(f"  reserved1        : {file_header.reserved1}")
    print(f"  reserved2        : {file_header.reserved2}")
    print(f"  bitmap_offset    : {file_header.bitmap_offset} (0x{file_header.bitmap_offset:X})")

    # --- DIB Info Header (40 bytes / BITMAPINFOHEADER) ---
    print("\n" + "=" * 45)
    print("      BITMAP INFO HEADER (40 bytes / V3)     ")
    print("=" * 45)
    print(f"  size (header)    : {info_header.size} bytes")
    print(f"  width            : {info_header.width} px")
    print(f"  height           : {info_header.height} px {'(bottom-up)' if info_header.height > 0 else '(top-down)'}")
    print(f"  planes           : {info_header.planes}")
    print(f"  bits_per_pixel   : {info_header.bits_per_pixel} bpp")
    print(f"  compression      : {info_header.compression} ({'BI_RGB uncompressed' if info_header.compression == 0 else 'compressed'})")
    print(f"  size_of_bitmap   : {info_header.size_of_bitmap:,} bytes")
    print(f"  horz_resolution  : {info_header.horz_resolution} px/m")
    print(f"  vert_resolution  : {info_header.vert_resolution} px/m")
    print(f"  colors_used      : {info_header.colors_used}")
    print(f"  colors_important : {info_header.colors_important}")
    print("=" * 45)


    # Validation checks
    errors = []

    if info_header.size != 40:
        errors.append(f"Not BMP V3 (header size is {info_header.size}, expected 40)")

    if info_header.bits_per_pixel != 24:
        errors.append(f"Not 24 bpp (found {info_header.bits_per_pixel} bpp)")

    if info_header.compression != 0:
        errors.append(f"Image is compressed (compression method is {info_header.compression}, expected 0/BI_RGB)")

    if not errors:
        print("\nThis file is OK")
    else:
        print("\nValidation failed:")
        for err in errors:
            print(f"  - {err}")


if __name__ == '__main__':
    main()

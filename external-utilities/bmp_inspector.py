import sys
import os
import ctypes
from pathlib import Path
from typing import Optional, Tuple


BASE_PATH = Path.cwd() / 'images'
HEADER_SIZE_BYTES = 54
TARGET_SIGNATURE = "0x4d42"

# Windows type aliases mapped to ctypes (all Little-Endian)
WORD = ctypes.c_uint16
DWORD = ctypes.c_uint32
LONG = ctypes.c_int32


class FileHeader(ctypes.LittleEndianStructure):
    _pack_ = 1                      # Disable compiler alignment padding
    _layout_ = 'ms'
    _fields_ = [
        ("file_type", WORD),       # 0x4D42 ('BM')
        ("file_size", DWORD),      # Size of file in bytes
        ("reserved1", WORD),       # 0
        ("reserved2", WORD),       # 0
        ("bitmap_offset", DWORD),  # Offset to pixel array
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
    bitmaps = list(filter(lambda x: x.lower().endswith('.bmp') , os.listdir(base_path)))
    print('Select bitmap:')
    for i, filename in enumerate(bitmaps):
        print(f"{i + 1:2d} --> {filename}")

    try:
        n = int(input(">>> "))
        if n < 1 or n > len(bitmaps):
            print('Invalid input')
        return bitmaps[n - 1]
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

    signature = f"{file_header.file_type:#06x}"
    print(f"Signature: {signature} ({"OK" if signature == TARGET_SIGNATURE else "ERROR"})")
    print(f"File Size: {file_header.file_size} bytes")
    print(f"Dimensions: {info_header.width}x{info_header.height}")
    print(f"Bit Depth: {info_header.bits_per_pixel} bpp")


if __name__ == '__main__':
    main()

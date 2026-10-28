#ifndef _BITMAP_V3_H_
#define _BITMAP_V3_H_

#include <stdint.h>

// WORD is defined by Microsoft as a 16-bit unsigned integer
//https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/f8573df3-a44a-4a50-b070-ac4c3aa78e3c
typedef uint16_t    WORD;

// DWORD is defined by Microsoft as a 32-bit unsigned integer
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/262627d8-3418-4627-9218-4ffe110850b2
typedef uint32_t    DWORD;

// LONG is defined by Microsoft as a 32-bit signed integer, in twos-complement format
// https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/29d44d70-382f-4998-9d76-8a1fe93e445c
typedef int32_t     LONG;

// 14-byte BITMAPFILEHEADER
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader
// We use __attribute__((packed)) GCC's flag to prevent compile-time padding on this structure, as it is mapped
// to a file header.
typedef struct  __attribute__((packed)) tagBITMAPFILEHEADER {
    WORD  bfType;               // The file type; must be 0x4d42 (the ASCII string "BM").
    DWORD bfSize;               // The size, in bytes, of the bitmap file.
    WORD  bfReserved1;          // Reserved; must be zero.
    WORD  bfReserved2;          // Reserved; must be zero.
    DWORD bfOffBits;            // The offset, in bytes, from the beginning of the BITMAPFILEHEADER structure to the bitmap bits.
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

// 40-byte BITMAPINFOHEADER
// https://learn.microsoft.com/en-us/previous-versions/dd183376(v=vs.85)
// We use __attribute__((packed)) GCC's flag to prevent compile-time padding on this structure, as it is mapped
// to a file header.
typedef struct __attribute__((packed)) tagBITMAPINFOHEADER {
    DWORD biSize;               // The number of bytes required by the structure.
    LONG  biWidth;              // The width of the bitmap, in pixels.
    LONG  biHeight;             // The height of the bitmap, in pixels. > 0 is bottom-up, < 0 is top-down.
    WORD  biPlanes;             // The number of planes for the target device. This value must be set to 1.
    WORD  biBitCount;           // The number of bits-per-pixel. Must be 24.
    DWORD biCompression;        // The type of compression for a compressed bottom-up bitmap. Must be 0.
    DWORD biSizeImage;          // The size, in bytes, of the image.
    LONG  biXPelsPerMeter;      // The horizontal resolution, in pixels-per-meter, of the target device for the bitmap.
    LONG  biYPelsPerMeter;      // The vertical resolution, in pixels-per-meter, of the target device for the bitmap.
    DWORD biClrUsed;            // The number of color indexes in the color table that are actually used by the bitmap.
    DWORD biClrImportant;       // The number of color indexes that are required for displaying the bitmap.
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#endif // _BITMAP_V3_H_

import os
from io import BufferedRandom
from dataclasses import dataclass


FILENAME = 'images/cat_v3.bmp' # 700 x 700 BMP V3
HEIGHT = 700
WIDTH = 700


@dataclass(frozen=True)
class Pixel:
    red: int
    green: int
    blue: int

    @classmethod
    def negative_of(cls, p: Pixel) -> Pixel:
        return Pixel(255 - p.red, 255 - p.green, 255 - p.blue)


def read_bmp_pixel(file: BufferedRandom, i: int, j: int, width: int) -> Pixel:
    offset = 3 * (-(i + 1) * width + j)
    file.seek(offset, os.SEEK_END)
    pixel_data: list[int] = list(file.read(3))
    return Pixel(pixel_data[2], pixel_data[1], pixel_data[0])



def modify_bmp_pixel(file: BufferedRandom, i: int, j: int, width: int, p: Pixel) -> None:
    offset = 3 * (-(i + 1) * width + j)
    file.seek(offset, os.SEEK_END)
    file.write(bytes((p.blue, p.green, p.red)))



def main():                                 # Test program to turn an image to its negative
    with open(FILENAME, 'r+b') as file:
        for i in range(HEIGHT):
            for j in range(WIDTH):
                p = read_bmp_pixel(file, i, j, WIDTH)
                modify_bmp_pixel(file, i, j, WIDTH, Pixel.negative_of(p))

main()

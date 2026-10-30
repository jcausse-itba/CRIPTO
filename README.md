<h1 align="center">72.44 - Criptografía y Seguridad</h1>
<h2 align="center">Trabajo Práctico de Implementación - stegobmp</h2>

## Grupo 10

| **Legajo** | **Apellido** | **Nombre**      | **Usuario GitHub**                                      | **Mail ITBA**                                         |
|------------|--------------|-----------------|---------------------------------------------------------|-------------------------------------------------------|
| 61195      | Causse       | Juan Ignacio    | [`jcausse`](https://github.com/jcausse)                 | [jcausse@itba.edu.ar](mailto:jcausse@itba.edu.ar)     |
| 64679      | Lanari       | Augusto Andrés  | [`augustolanari`](https://github.com/augustolanari)     | [alanari@itba.edu.ar](mailto:alanari@itba.edu.ar)     |
| 64332      | Liu          | Javier Emmanuel | [`JaviertoZerrado`](https://github.com/JaviertoZerrado) | [jaliu@itba.edu.ar](mailto:jaliu@itba.edu.ar)         |
| 64292      | Rivas        | Nicolás         | [`nrivas-itba`](https://github.com/nrivas-itba)         | [nrivas@itba.edu.ar](mailto:nrivas@itba.edu.ar)       |

## Contenido

- [Tecnologías Utilizadas](#tecnologías-utilizadas)
- [Instrucciones de Compilación](#instrucciones-de-compilación)
  - [Compilación para Entrega (Release)](#compilación-para-entrega-release)
  - [Compilación para Desarrollo (Dev)](#compilación-para-desarrollo-dev)
  - [Otras Opciones de Compilación](#otras-opciones-de-compilación)
- [Bitmap Inspector](#bitmap-inspector)
  - [Convertir Bitmaps a V3](#convertir-bitmaps-a-v3)
- [Instrucciones de Ejecución](#instrucciones-de-ejecución)
- [Instrucciones de Testing](#instrucciones-de-testing)

## Tecnologías Utilizadas

- C23 (ISO/IEC 9899:2024)
- Librería OpenSSL
- Python 3.x ([bitmap inspector](#bitmap-inspector))

## Instrucciones de Compilación

> **Nota:**
> 
> Puede ver el detalle completo de las opciones disponibles en el `Makefile` ejecutando `make`.

Debe contar con:
- `GCC` (versión 14 o superior con soporte para C23).
- `make`.
- `CMake` (versión 3.21 o superior).
- Librería `OpenSSL`.
- [Librerías `ASan` y `UBSan`](https://github.com/google/sanitizers) (solo desarrollo).

<details><summary>Desplegar para ver instrucciones de instalación</summary>

Puede instalar estas dependencias en distribuciones basadas en Debian/Ubuntu mediante:
```shell
sudo apt update -y && sudo apt install -y gcc make cmake openssl libssl-dev build-essential
```

</details>

### Compilación para Entrega (Release)
Genera el ejecutable optimizado sin AddressSanitizer (ASan) ni sanitizers para reducir el consumo de memoria y maximizar el rendimiento:
```shell
make release
```

### Compilación para Desarrollo (Dev)
Genera el ejecutable de desarrollo incluyendo AddressSanitizer (ASan) y UndefinedBehaviorSanitizer (UBSan) para depuración y detección temprana de errores de memoria:
```shell
make dev
```

### Otras Opciones de Compilación

<details><summary>Desplegar para ver otras opciones de compilación</summary>

### CMake por línea de comandos

Modo Release (sin sanitizers):
```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF
cmake --build build --target stegobmp
```

Modo Debug (con sanitizers):
```shell
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build --target stegobmp
```

### CLion
Simplemente abra el directorio del proyecto en CLion. Al detectar `CMakeLists.txt`, CLion configurará el proyecto automáticamente y creará los perfiles de compilación y ejecución para `stegobmp` y los tests.

</details>

## Bitmap Inspector

Por requisito de enunciado, `stegobmp` solamente soporta Bitmaps V3 (versión 3), con un encabezado doble de 54 bytes en total, compuestos de:
- [14 bytes de encabezado de archivo (conocido como `BITMAPFILEHEADER`)](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapfileheader).
- [40 bytes de información sobre el Bitmap (conocido como `BITMAPINFOHEADER`)](https://learn.microsoft.com/en-us/previous-versions/dd183376(v=vs.85)).

Sin embargo, es muy común encontrar Bitmaps con encabezado [`BITMAPV5HEADER`](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header) en lugar de `BITMAPFILEHEADER`.

Además, `stegobmp` solamente soporta bitmaps de 24 bits por píxel, no comprimidos.

Si se utiliza un Bitmap que no cumpla estos requisitos, el comportamiento de `stegobmp` puede no ser el esperado. Debido a esto, se provee `Bitmap Inspector`, un script de Python que permite verificar que el archivo `.bmp` sea adecuado para utilizar con `stegobmp`. Para ejecutarlo:

```shell
make inspect
```

El programa escaneará el directorio `images/` en la raíz del proyecto en busca de archivos `.bmp` y le permitirá seleccionar un archivo para inspeccionar. 

Si se inspecciona un Bitmap inválido, se mostrará un error. Ejemplo con un Bitmap versión 5:

```text
juani@Spectre:~/CRIPTO$ make inspect
Select bitmap:
 1 --> eiffel_tower_v3.bmp
 2 --> cat_v3.bmp
 3 --> cat_v5.bmp
>>> 3
  
Selected file: /home/juani/CRIPTO/images/cat_v5.bmp
  
Validation failed:
  - Not BMP V3 (header size is 124, expected 40)
```

Al inspeccionar un Bitmap válido, el programa mostrará `This file is OK`, y le permitirá mostrar los encabezados `BITMAPFILEHEADER` y `BITMAPINFOHEADER`. Ejemplo:

```text
juani@Spectre:~/CRIPTO$ make inspect
Select bitmap:
 1 --> eiffel_tower_v3.bmp
 2 --> cat_v3.bmp
 3 --> cat_v5.bmp
>>> 2

Selected file: /home/juani/CRIPTO/images/cat_v3.bmp

This file is OK
Show headers? (y/n): y
=============================================
           BMP FILE HEADER (14 bytes)        
=============================================
  file_type        : 0x4d42 ('BM') [OK]
  file_size        : 1,470,054 bytes
  reserved1        : 0
  reserved2        : 0
  bitmap_offset    : 54 (0x36)

=============================================
      BITMAP INFO HEADER (40 bytes / V3)     
=============================================
  size (header)    : 40 bytes
  width            : 700 px
  height           : 700 px (bottom-up)
  planes           : 1
  bits_per_pixel   : 24 bpp
  compression      : 0 (BI_RGB uncompressed)
  size_of_bitmap   : 1,470,000 bytes
  horz_resolution  : 0 px/m
  vert_resolution  : 0 px/m
  colors_used      : 0
  colors_important : 0
=============================================
```

### Convertir Bitmaps a V3

Dado un Bitmap incompatible pero no comprimido, se puede convertir a uno V3 compatible en Linux utilizando `imagemagick`:

Instalación:

```shell
sudo apt install imagemagick
```

Conversión:

```shell
magick convert <input file>.bmp BMP3:<output file>.bmp
```

Ejemplo:

```shell
magick convert cat_v5.bmp BMP3:cat_v3.bmp
```

## Instrucciones de Ejecución

> **Nota:**
>
> Por simplicidad, las imágenes provistas en `image/` son Bitmaps V3 compatibles, verificadas con [`Bitmap Inspector`](#bitmap-inspector). Puede utilizar sus propios BMP verificándolos primero para asgurar su compatibilidad, y [convirtiéndolos con `imagemagick`](#convertir-bitmaps-a-v3) de ser necesario.

TODO

## Instrucciones de Testing

Para ejecutar todos los tests de manera automática, ejecute:
```shell
make test
```

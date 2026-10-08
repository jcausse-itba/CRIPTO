<h1 align="center">72.44 - Criptografía y Seguridad</h1>
<h2 align="center">Trabajo Práctico de Implementación</h2>

## Grupo 10

| **Legajo** | **Apellido** | **Nombre**      | **Usuario GitHub**                                      | **Mail ITBA**                                         |
|------------|--------------|-----------------|---------------------------------------------------------|-------------------------------------------------------|
| 61195      | Causse       | Juan Ignacio    | [`jcausse`](https://github.com/jcausse)                 | [jcausse@itba.edu.ar](mailto:jcausse@itba.edu.ar)     |
| 64679      | Lanari       | Augusto Andrés  | [`augustolanari`](https://github.com/augustolanari)     | [alanari@itba.edu.ar](mailto:alanari@itba.edu.ar)     |
| 64332      | Liu          | Javier Emmanuel | [`JaviertoZerrado`](https://github.com/JaviertoZerrado) | [jaliu@itba.edu.ar](mailto:jaliu@itba.edu.ar)         |
| 64292      | Rivas        | Nicolás         | [`nrivas-itba`](https://github.com/nrivas-itba)         | [nrivas@itba.edu.ar](mailto:nrivas@itba.edu.ar)       |

## Contenido

- [Tecnologías Utilizadas](#tecnologías-utilizadas)
- [Instrucciones de Compilación](#instrucciones-de-compilacion)
- [Instrucciones de Ejecución](#instrucciones-de-ejecucion)
- [Instrucciones de Testing](#instrucciones-de-testing)

## Tecnologías Utilizadas

- C23 (ISO/IEC 9899:2024)
- Librería OpenSSL

## Instrucciones de Compilación

Debe contar con:
- `GCC` (versión 14 o superior con soporte para C23)
- `make`
- `CMake` (versión 3.21 o superior)

<details><summary>Desplegar para ver instrucciones de instalación</summary>

Puede instalar estas dependencias en distribuciones basadas en Debian/Ubuntu mediante:
```shell
sudo apt update -y && sudo apt install -y gcc make cmake
```

</details>

Si cuenta con dichas dependencias, simplemente ejecute:
```shell
make
```

<details><summary>Desplegar para ver otras opciones de compilación</summary>

### CLion
Simplemente abra el directorio del proyecto en CLion. Al detectar `CMakeLists.txt`, CLion configurará el proyecto automáticamente y creará los perfiles de compilación y ejecución para `stegobmp` y los tests.

### CMake por línea de comandos
```shell
cmake -B build
cmake --build build
```

</details>

## Instrucciones de Ejecución

TODO

## Instrucciones de Testing

Para ejecutar todos los tests de manera automática, ejecute:
```shell
make test
```

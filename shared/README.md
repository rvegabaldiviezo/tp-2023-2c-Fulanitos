# Shareds Library For C

Biblioteca con funciones hechas en lenguaje C y útiles para el desarrollo del TP de SO.

Provee los siguientes TADs:

* Crear un cliente o servidor (shared/socket.h)
* Serializacion de estructuras y datos (shared/serialization.h)
* Estructuras del TP (shared/structures.h)

## Notas

Algunas de las consideraciones a tener a la hora de su uso:

* Ninguna de las implementaciones utiliza semáforos, por lo que el uso concurrente debe ser implementado por el usuario de estas.
* Ninguna de las funciones implementadas posee validaciones para manejo de errores.

## Guía de Instalación

Instrucciones de instalación:

- Ir al directorio del Makefile
```bash
cd src
```

- Ejecutar el comando Makefile que necesita

    - `make uninstall` -> desinstala la biblioteca

    - `make install` -> instala la biblioteca en el sistema


## Guía para el uso

1. Linkear con `-lshared`

2. Para usarla en un .c/.h deberá incluirse de la siguiente forma: `shared/<Nombre_TAD>`

Por ejemplo:

```c
#include <shared/socket.h>
#include <shared/serializacion.h>
#include <shared/structures.h>
```

### Desde Eclipse

1. Ir a las Properties del proyecto (en el Project Explorer - la columna de la izquierda - la opción aparece dándole click derecho al proyecto), y dentro de la categoría `C/C++ Build` entrar a `Settings`, y ahí a `Tool Settings`.
2. Buscar `GCC Linker` > `Libraries` > `Libraries`. Notar que entre paréntesis dice `-l`, el parámetro de `gcc` que estamos buscando.
3. Darle click en el botón de `+`, y poner el nombre de la biblioteca sin el `-l` (en este caso, `shared`).
4. Aceptar y buildear el proyecto.
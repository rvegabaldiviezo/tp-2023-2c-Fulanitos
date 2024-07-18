# Info extra


## Contenido relacionados del TP con la materia sistemas operativos 

|         TEMAS               |          LINK                   |
|------------------------|--------------------|   
| Programación en C  | [Buenas Prácticas de C](https://docs.utnso.com.ar/guias/programacion/buenas-practicas.html#buenas-practicas-de-c), [Introducción al Lenguaje C](https://docs.utnso.com.ar/primeros-pasos/lenguaje-c.html#introduccion-al-lenguaje-c), [Makefile - Informática I - FRBA - UTN](https://www.youtube.com/watch?v=A35l4jXBvEY&ab_channel=InformaticaI-UTN-FRBA) ,[Funciones en Lenguaje C - Informática I - FRBA - UTN](https://www.youtube.com/watch?v=aciC4izEiCo&ab_channel=InformaticaI-UTN-FRBA) ,[Estructuras y uniones - Informática I - FRBA - UTN](https://www.youtube.com/watch?v=Tw8PmTRuU_Q&ab_channel=InformaticaI-UTN-FRBA), [Variables y constantes en Lenguaje C - Informática I - FRBA - UTN](https://www.youtube.com/watch?v=DlG2K674O1E&ab_channel=InformaticaI-UTN-FRBA), [makefile : cómo construirlo](https://www.youtube.com/watch?v=0XlVyZAfQEM&t=1066s&ab_channel=WhileTrueThenDream), [Funciones de orden superior de las commons con más parámetros](https://www.youtube.com/watch?v=1kYyxZXGjp0&list=PL6oA23OrxDZDSe0ziMJ7iE-kPq9PdonPx&index=16&ab_channel=UTNSO) |
| Tests unitarios en C  | [Unit Testing con CSpec](https://docs.utnso.com.ar/guias/herramientas/cspec.html#unit-testing-con-cspec), [C - Unit Testing Introduction with Google Test](https://www.youtube.com/watch?v=BwO07hUzFNQ&ab_channel=KrisJordan) |
| Biblioteca compartida en C | [Crear una Biblioteca Compartida desde Eclipse 2020 + GCC Internals](https://www.youtube.com/watch?v=A6dhc9cCI18&ab_channel=UTNSO), [C - Shared Library](https://www.youtube.com/watch?v=Aw9kXFqWu_I&list=PL6oA23OrxDZDSe0ziMJ7iE-kPq9PdonPx&index=3&ab_channel=UTNSO) |
| Sockets  | [3ra Charla 1c 2019 - Sockets](https://www.youtube.com/watch?v=V0KFn9w62sY&ab_channel=UTNSO), [Sockets TCP. Linux. C: Ejemplo servidor secuencial.Breve teoría](https://www.youtube.com/watch?v=zFHjKCVwS48&t=1s&ab_channel=WhileTrueThenDream) |
| Serialización | [3ra Charla 1c 2019 - Serializacion](https://www.youtube.com/watch?v=GnuurOt8yqE&ab_channel=UTNSO), [Serialización](https://www.youtube.com/watch?v=gXr-zawbhIY&list=PLSwjRgubz0MaiiBb426tJxQoyIikVsNWK&index=7&ab_channel=LaCajadeUTN)|
| POSIX Threads | [Hilos y mutex en C usando pthread](https://www.youtube.com/watch?v=gl8FQU3VEzU&ab_channel=UTNSO),   [3ra Charla 1c 2019 - Threads](https://www.youtube.com/watch?v=G8PD6wauMeY&t=1770s&ab_channel=UTNSO), [Programar en C con Hilos pthreads](https://www.youtube.com/watch?v=NAKrOZCcJ4A&t=208s&ab_channel=WhileTrueThenDream) |
| Concurrencia | [video](), [Mutex. Sincronización de hilos. Programar en C](https://www.youtube.com/watch?v=faZEhIHdJx8&t=12s&ab_channel=WhileTrueThenDream)|
| Semáforos | [video]() , [3ra Charla 1c 2019 - Threads](https://www.youtube.com/watch?v=G8PD6wauMeY&t=1770s&ab_channel=UTNSO)|
| Gestión de memoria | [video](), [3ra Charla 1c 2019 - Valgrind](https://www.youtube.com/watch?v=knRei6OBU4Q&ab_channel=UTNSO) | 
| Segmentación Paginada |  [video]() |
| Planificación de procesos |  [Planificación - 2C2020](https://www.youtube.com/watch?v=SQsC7bwt3_c&ab_channel=UTNSO), [UTN 20 - SO: Procesos y Planificacion](https://www.youtube.com/watch?v=iOZLnOXQxVE&ab_channel=Snoopy4k),  [UTN 20 - SO, Teoria: Planificación de CPU ALgoritmos](https://www.youtube.com/watch?v=4J7hEXekn4M&ab_channel=Snoopy4k) |
| Multiprogramación |  [Multiprogramación, Gestión de memoria, Procesos](https://www.youtube.com/watch?v=oeuGAxxovxs&ab_channel=TelesensesSenses) |
| Multiprocesamiento | [UTN 20 - Sstemas Operativos, Teoria: Multicomputadoras y Sistemas Distribuidos](https://www.youtube.com/watch?v=yaKKhdeQ7FU&ab_channel=Snoopy4k) |
| Sistema de archivos |  [video]() |
| Bash Scripting |  [Guía de uso de Bash](https://docs.utnso.com.ar/guias/consola/bash.html#guia-de-uso-de-bash) |
| Automatización de deploy | [Guía de despliegue de TP](https://docs.utnso.com.ar/guias/herramientas/deploy.html#guia-de-despliegue-de-tp) |




## Instrucciones de configuración iniciales

Aquí se encuentran las instrucciones detalladas para configurar el proyecto.

#### **1.Clonar y acceder al Repositorio del TP**

- **Clonar el repo**

    - ssh: Es necesario generar en la VM el .ssh y subir a github tu id .pub, ejemplo: /home/utnso/.ssh/id_ecdsa.pub  
    ```bash
    git clone git@github.com:sisoputnfrba/tp-2023-2c-Fulanitos.git
    ```
    
    - https: Es necesario generar tu personal access token github y con el username de github te validas (Repo privado).

    ```bash
    git clone https://github.com/sisoputnfrba/tp-2023-2c-Fulanitos.git
    ```

- **Entrar al repositorio**
    ```bash
    cd tp-2023-2c-Fulanitos/
    ```

#### **2.Descargar e instalar las dependecias**

- **Instalar y compilar las Commons-Library (librerias creadas por la catedra de SO)**
    ```bash
    make commons
    ```
- **Instalar y compilar las Shared-Library (librerias creadas por el equipo)**
    ```bash
    make shared
    ```

#### **3.Mapear las IP de todos los modulos**
Es guardar las IP de cada modulo en el archivo ./config/ip.config

- **Ejecutar mapeos de IPs de los modulos restantes**
    ```bash
    make ips
    ```
(Esto no esta implementado, es solo una idea)



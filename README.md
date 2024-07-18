# tp-2023-2c-Fulanitos

## ENUNCIADO 

[Manejo Planificado de Procesos y Archivos](https://docs.google.com/document/d/1g6DEcbjilpX2XUBADuF6dPnrLv5lzoTZSVYCPgpHM_Q/edit)

El objetivo del trabajo práctico consiste en desarrollar una solución que permita la simulación de un sistema distribuido, planificando procesos, resolviendo las peticiones al sistema y administrando la memoria. Esta simulacion nos permite aplicar de forma práctica los conceptos teóricos dictados en la cursada.


## GRUPO: Fulanitos

| Apellido y Nombre | GitHub user | Módulos a cargo |
|-------------------|-------------|-----------------|
| LEOSENCO, Juan Cruz | [@Jcleosenco](https://github.com/Jcleosenco)| ... |
| MOROSINI, Pablo | [@MorosiniP](https://github.com/MorosiniP) | ... |
| NOBLIA, Francisco | [@elpanchi05](https://github.com/elpanchi05) | ... |
| GIL T., Augusto Adrian  | [@adriangilt02](https://github.com/adriangilt02)| ... |
| VEGA B., Ramon Angel  | [@rvegabaldiviezo](https://www.github.com/rvegabaldiviezo) | ... |


## INSTRUCCIONES INICIALES DE CONFIGURACION 
[Instrucciones de configuración](./INSTRUCCIONES.md#instrucciones-de-configuración-iniciales)

## INSTRUCCIONES DE EJECUCION

### Compilar todos los Modulos y las Shared
```bash
make all
```
### Ejecutar un modulo
```bash
./run [modulo] [config] 
```
[modulo]: Nombre del modulo (kernel, cpu, fileSystem, memoria)

[config]: Cuando no se pasa una configuracion, se usa la configuracion **default** 

#### Ejemplos

- **memoria**
    ```bash
    ./run memoria
    ``` 
	```bash
    ./run memoria base    
	```  
- **fileSystem**
    ```bash
    ./run fileSystem
    ``` 
	```bash
    ./run fileSystem base    
	```  
- **kernel**
    ```bash
    ./run kernel
    ``` 
	```bash
    ./run kernel base    
	```  
- **cpu**
    ```bash
    ./run cpu
    ``` 
	```bash
    ./run cpu base    
	```  
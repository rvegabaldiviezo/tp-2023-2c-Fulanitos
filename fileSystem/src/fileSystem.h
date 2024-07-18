#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>  
#include <shared/socket.h>
#include <shared/serialization.h>
#include <shared/setup.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <shared/structures.h>  
#include <commons/bitarray.h>
#include <stdbool.h> 
#include <commons/memory.h> 
#include <math.h>
#include <inttypes.h>

void initialize_sockets();
void initialize_fsfiles();
void escribir_archivo_de_bloques(uint32_t ptr, void* contenido);
void leer_bloque_swap(uint32_t nro_bloque, void* contenido);
void actualizar_tamanio_fcb(t_fcb* fcb);
void liberar_fcb(t_fcb* fcb);
void actualizar_bloque_inicial_fcb(t_fcb* fcb);
void liberar_bloques_en_fat(t_fcb* fcb, int bloques_a_liberar, uint32_t bloques_actuales);
void* atender_cliente(void* socket_cliente);
int recibir_codigo_operacion(int socket_cliente);
void* atender_kernel();
void* atender_memoria();
int verificar_existencia_fcb(char* nombre_archivo);
t_fcb* obtener_fcb(char* nombre_archivo);
char* concatenar_archivo_fcb_con_carpeta(char* nombre_archivo);
void asignar_bloques_en_fat(t_fcb* fcb, int bloques_a_asignar);
void liberar_bloques_en_fat(t_fcb* fcb, int bloques_a_liberar, uint32_t bloques_actuales);
void leer_bloque(uint32_t ptr_al_bloque, void* contenido, char* archivo);
void escribir_bloque(uint32_t ptr, void* contenido, char* archivo);
void actualizar_tamanio_archivo(char* nombre_archivo, int tamanio_a_actualizar);
t_fcb* obtener_fcb(char* nombre_archivo);
void liberar_estructuras();
void crear_fcb(char* nombre_archivo);
void start_archivo_de_bloques();
void start_tabla_fat();
void initialize_fsfiles();
void start_carpeta_fcb();
void reservar_bloques_de_swap(int cant_bloques_swap_a_reservar, uint32_t*);
void liberar_bloques_swap(uint32_t* bloques_swap_a_liberar, int n_bloques);
int obtener_bloque_inicial(char* nombre_archivo);
uint32_t leer_entrada_fat(uint32_t entrada);
void crear_bitarray_desde_archivo(t_bitarray** bitarray, char* path, int tamanio);
void persistir_bitarray(char* path, t_bitarray* puntero_bitarray_a_persistir, int cant_bloques);
void escribir_bloque_swap(uint32_t nro_bloque, void* contenido);
uint32_t buscar_en_fat(int bloque_inicial, uint32_t bloque_buscado);

t_log* logger;
t_config* fileSystem_config;
int socket_filesystem;
int socket_memoria;
int socket_cliente;
t_buffer* buffer;
int cant_total_bloques;
int cant_bloques_swap;
int cant_bloques_fat;
uint32_t* tabla_fat;
int tam_tabla_fat;
FILE *bloques_file;
FILE* fcb_file;
FILE* fat_file;
int tam_bloques;
char* path_carpeta_fcbs;
int comienzo_particion_fat;
t_bitarray* bloques_libres_swap;
char* puntero_bitarray;
t_bitarray* bloques_libres_fat;
char* puntero_bitarray_fat;

#endif /* FILESYSTEM_H_ */
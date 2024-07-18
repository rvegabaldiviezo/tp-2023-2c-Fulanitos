#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <shared/serialization.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/memory.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <shared/socket.h>
#include <shared/structures.h>
#include <shared/setup.h>
#include <libgen.h>

typedef struct {
    t_entrada_tp* pagina_victima;
    t_list* tabla_victima;
    int pid_victima;
} t_victima;

extern const char* instruction_strings[];
t_log *logger;
t_config *memoria_config;

void* memoria_usuario;
t_list* procesos;
t_bitarray* marcos_libres;
t_list* proximas_victimas;

int tam_memoria;
int tam_paginas;
int* error;
int socket_filesystem;
int socket_cpu;
int socket_kernel;
int socket_memoria;
int id;
int tiempo_espera_ms;
int cant_marcos;
int cant_marcos_utilizados;
char* algoritmo_reemplazo;
t_list* tabla_victima;
t_entrada_tp* pagina_victima;
int pid_victima;
int pid_comparador;
int retardo_en_ms;
t_entrada_tp* pagina_aux;

sem_t mutex_memoria;
sem_t mutex_procesos;
sem_t mutex_pid_comparador;
sem_t mutex_pagina_aux;

void inicializar_estructuras_memoria();
bool distinta_pagina(void* element);
int buscar_marco_libre();
void finalizar_proceso(int id_proceso);
void* leer_del_espacio_usuario(uint32_t dir_fisica);
void escribir_en_espacio_usuario(uint32_t direccion_fisica, void* valor, size_t tamanio);
void initialize_sockets();
void liberar_setup();
void get_instruccion();
bool pagina_presente(void* data);
int acceder_a_tp_del_proceso(int pid, int nro_pagina);
void* atender_cliente(int socket);
void pedir_swap(t_proceso* proceso, int bloques_necesarios);
void crear_proceso(char* filename, int size);
bool memoria_vacia();
void handle_pf(t_proceso* proceso, int nro_pagina);
int seleccionar_pagina_victima_y_reemplazar(t_proceso* proceso, int nro_pagina);
void traer_pagina_a_memoria(t_proceso* proceso, int nro_pagina, void* contenido);
void escribir_en_memoria(int marco, void* contenido);
t_proceso* buscar_proceso_por_pid(int pid);
bool igual_pid(t_proceso* p);
void destruir_instrucciones(t_proceso* p);
void liberar_tabla(t_proceso* p);
void liberar_swap(t_proceso* p);
t_instruction* obtener_instruccion(int id, int pc);
char* concatenar_path_con_carpeta_de_instrucciones(char* path);
void enviar_bloque_de_swap_a_leer_fs(int pid, uint32_t nro_pag);
void* comparar_minimo(void* arg1, void* arg2);
void mandar_a_escribir_bloques_swap_fs(uint32_t nro_bloque, void* contenido);
int obtener_nro_pag_en_tp(t_list* tp, t_entrada_tp* entrada_buscada);
void liberar_procesos();

#endif 
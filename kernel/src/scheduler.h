#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <shared/socket.h>
#include <semaphore.h>
#include <shared/serialization.h>
#include <shared/structures.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
#include <time.h>
#include <shared/structures.h>
#include <libgen.h>

typedef struct {
    t_pcb* pcb;
	int socket;
} arg_fs;

typedef struct {
    t_pcb* pcb;
	int tiempo;
} arg_sleep;

typedef struct {
    entrada_archivo_global* archivo;
    t_pcb* pcb;
} arg_lock;

typedef struct {
    t_pcb* pcb;
    char* motivo;
} arg_fin;

char** recursos;
	char** instancias_recursos;
	t_list** colas_recursos;
t_list* plani_new;
	t_list* plani_ready;
	t_list* total_pcbs;
	t_list* plani_exec;
	t_list* plani_block;
	t_list* plani_exit;
algoritmo algoritmo_plani;
int multiprogramacion;
int tiempo_quantum;
t_log* logger;
t_config* config;
sem_t multiprog_disponible;
	sem_t mutex_multiprog;
	sem_t mutex_multiprog_exit;
	sem_t pcbs_en_new;
	sem_t pcbs_en_ready;
	sem_t puede_ejecutar;
	sem_t debug;
	sem_t aguardar_deadlock;

	sem_t sem_total_pcbs;
	sem_t sem_new;
	sem_t sem_ready;
	sem_t sem_exec;
	sem_t sem_block;
	sem_t sem_exit;

	sem_t sem_recursos;
	sem_t sem_inst_recursos;
	sem_t sem_block_recursos;

	sem_t sem_detener_execute;
	sem_t sem_detener_new_ready;
	sem_t sem_detener_block_ready;
	sem_t sem_detener_block;
	sem_t sem_detener_planificacion;

	sem_t mutex_socket_fs;
int socket_cpu_plani;
	int socket_cpu_plani_int;
	int socket_memoria_plani;
	int socket_fs_plani;
t_list* tabla_archivos_global;

//FUNCIONES INCIALIZACION
void* crear_proceso(char* path, int size,int prioridad);
void initialize_scheduler(t_config* config, t_log* logger, int cpu_int, int cpu, int memoria, int fs);
void inicio_semaforos();
void detener_planificacion();
void iniciar_planificacion();
void listar_estados();
void listar_general(t_list* lista, char* estado);
void crear_nuevo_semaforo_multiprog(int nuevo_grado);
//PLANIFICADOR LARGO PLAZO
void* inicio_plani_largo_plazo();
//PLANIFICADOR CORTO PLAZO
void agregar_a_new(t_pcb* nuevo_pcb);
void agregar_a_ready(t_pcb* pcb);
void* inicio_plani_corto_plazo();
void sacar_uno_de_exec();
void* iniciar_quantum(void* arg);
entrada_archivo_global* crear_entrada_global(char* archivo);
entrada_archivo_global* buscar_archivo_tabla_global(char* archivo);
entrada_archivo* crear_entrada_tabla_pcb(t_pcb* pcb, entrada_archivo_global* archivo);
void borrar_entrada_global(entrada_archivo_global* e);
void borrar_entrada_tabla_pcb(t_pcb* pcb, char* archivo);
void* bloquear_proceso_sleep(void*);
void crear_hilo_bloqueo(t_pcb*, int);
int recibir_tiempo_de_bloqueo();
size_t recibir_tamanio_recurso();
char* recibir_parametro(int);
char* estado_a_string(t_estado);
t_pcb* sacar_siguiente_de_new();
int total_recursos();
void sacar_de_block_recurso(int recurso);
void* aguardar_solucion_deadlock(void* indice);
void ciclo_deteccion_deadlock();
void detectar_deadlock_archivos();
void recibir_contexto_y_actualizar_pcb(t_pcb* pcb);
t_pcb* pcb_de_exec();
t_pcb* sacar_de_exec();
t_pcb* sacar_de_ready();
t_pcb* sacar_de_block(t_pcb* pcb);
void mostrar_log_agregado_ready();
t_pcb* sacar_de_exec();
void agregar_a_block(t_pcb* pcb);
t_pcb* sacar_de_block_por_id(int pid);
void agregar_a_exit(void* arg);
void agregar_a_exec(t_pcb* pcb);
void finalizar_proceso(t_pcb* pcb, char* motivo);
void cambiar_estado_pcb(t_pcb* pcb, t_estado estadoNuevo);
void* comparar_prioridad(void* arg1, void* arg2);
int posicion_del_recurso(char* recurso);
void agregar_a_block_recurso(t_pcb* pcb, int recurso);
bool es_new(t_pcb* pcb);
bool es_ready(t_pcb* pcb);
bool es_blocked(t_pcb* pcb);
bool es_execute(t_pcb* pcb);
bool es_exit(t_pcb* pcb);
bool igual_pid(t_pcb* pcb);
void crear_proceso_memoria(char* path, int size);
void enviar_interrupcion(int pid, t_int interrupcion);
void sacar_de_ready_particular(t_pcb* pcb);
void sacar_de_new_particular(t_pcb* pcb);
t_pcb* buscar_pcb_por_pid(int pid);
void liberar_recursos_asignados(t_pcb* pcb);
void borrar_tabla_archivos_abiertos_pcb(t_pcb* pcb);
void liberar_memoria(t_pcb* pcb);
void handle_pf(void* args);
void solicitar_pagina(int pagina, int pid);
void actualizar_pcb(t_pcb* pcb, t_contexto* contexto);

void detectar_deadlock_recursos();
void hay_deadlock(t_list* procesos);
char* archivos_afectados(t_pcb* pcb);
bool puede_finalizar(t_pcb* pcb);
void filtrar_sin_recursos(t_list* procesos, int* recursos_disponibles);
void liberar_recursos_deadlock(t_pcb* proceso, int* recursos_disponibles);
bool bloqueado_por_recurso(void* data);
void copiar_vector(int source[], int destination[], int size);
int sumar_vector(int* array);

bool crear_lock_lectura(entrada_archivo_global* archivo, t_pcb* pcb);
void* encolar_lock_lectura(void* arg);
void eliminar_lock_lectura(entrada_archivo_global* archivo);
bool crear_lock_escritura(entrada_archivo_global* archivo, t_pcb* pcb);
void* encolar_lock_escritura(void* arg);
void eliminar_lock_escritura(entrada_archivo_global* archivo);
void inicializar_locks(entrada_archivo_global* archivo);
entrada_archivo* buscar_archivo_tabla_pcb(t_list* tabla, char* archivo);
void* esperar_fs(void* arg);
#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#ifndef __STRUCTURES_H
#define __STRUCTURES_H

typedef enum {
    SET,
    SUM,
    SUB,
    JNZ,
    SLEEP,
    WAIT,
    SIGNAL,
    MOV_IN,
    MOV_OUT,
    F_OPEN,
    F_CLOSE,
    F_SEEK,
    F_READ,
    F_WRITE,
    F_TRUNCATE,
    EXIT,
} t_instruction_type;

typedef struct {
	t_instruction_type operation;
	t_list* parameters;
} t_instruction;

typedef struct {
    int size;
    void* stream;
} t_buffer;

typedef enum {
	AX,
	BX,
	CX,
	DX,
} t_register;

typedef enum {
    FIFO,
    RR,
    PRIORIDADES
} algoritmo;

typedef enum {
	HANDSHAKE,
    MENSAJE,
    OK,
    STRING,
    INSTRUCCION,
    CONTEXTO,
    PROCESS_STARTED,
    PROCESS_FINISHED,
    RAM_ACCESS_READ,
    RAM_ACCESS_WRITE,
    NO_SUCH_FILE,
    OPEN,
    READ,
    READ_MEM,
    WRITE_MEM,
    CLOSE,
    TRUNCATE,
    WRITE,
    CREATE,
    SEEK,
    SOLICITAR_VALOR,
    PAGE_FAULT,
    DESALOJO,
    SLEEP_BLOQUEANTE,
    SOLICITAR_WAIT,
    SOLICITAR_SIGNAL,
    CREAR_PROCESO,
    INTERRUPCION,
    NO_ASIGNADO,
    ESCRIBIR_SWAP,
    LEER_SWAP,
    BLOQUEADO,
} op_code;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCK,
	FIN
} t_estado;

typedef enum {
    SIN_INT,
    FIN_PROCESO,
    FIN_QUANTUM,
    DESALOJO_PRIORIDADES,
} t_int;

typedef struct {
    int id;
    int prioridad;
	unsigned int size;
	unsigned int program_counter;
    uint32_t registros[4];
    t_int interrupcion; 
    t_list* tabla_archivos_abiertos;
    t_list* tabla_global_archivos;
    t_estado estado;
    int* recursos_asignados;
    int* recursos_pedidos;
    char* path;
    bool block_recurso;
    bool block_archivo;
    char* recurso_requerido;
    bool block_pf;
    bool manejando_contexto;
    bool fin_por_consola;
} t_pcb;

typedef struct {
	char* archivo; 
    pthread_mutex_t lock;
    pthread_cond_t cola_lectura;
    pthread_cond_t cola_escritura;
    int lectores;
    int escritores;
} entrada_archivo_global;

typedef struct {   
    char* archivo;
    char modo;
	entrada_archivo_global* puntero_a_tabla_global;
    uint32_t puntero_en_archivo;
} entrada_archivo;

typedef struct {
    char* nombre_archivo;
    int tamanio_archivo;
    uint32_t bloque_inicial;
} t_fcb;

typedef enum {
    RECURSOS,
    ARCHIVOS
} tipo_deadlock;

typedef enum {
    INICIAR_PROCESO,
    FINALIZAR_PROCESO,
    DETENER_PLANIFICACION,
    INICIAR_PLANIFICACION,
    MULTIPROGRAMACION,
    PROCESO_ESTADO,
} t_funcion;

typedef struct {
    char* recurso;
    int instancias;
} t_recurso;

typedef struct{
    int id;
    unsigned int pc; 
    uint32_t registros[4];
    t_int interrupcion;  
} t_contexto;

typedef struct {
    op_code codigo_operacion;
    t_buffer* buffer;
}t_paquete;

typedef struct {
    int id;
    t_list* instrucciones;
    t_list* tp;
    uint32_t bloques[];
} t_proceso;

typedef struct {
    int marco;      
    int presente;       
	int modificado; 
    int ultimo_acceso;
    uint32_t nro_bloque;
} t_entrada_tp;

#endif
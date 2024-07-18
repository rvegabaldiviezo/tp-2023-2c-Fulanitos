#ifndef CPU_H_
#define CPU_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <shared/custom_logs.h>
#include <shared/environment_variables.h>
#include <shared/serialization.h>
#include <shared/socket.h>
#include <shared/structures.h>
#include <shared/setup.h>

t_log* logger; 
t_config* cpu_config;
int socket_cpu_dispatch,
	socket_cpu_interrupt,
	socket_kernel_dispatch,
	socket_kernel_interrupt,
	socket_memoria;
pthread_t thread_interrupt;
int interrupt_pid;
t_int interrupt_type;
int tam_pagina;

const char* instruction_strings[] = {
    "SET",
    "SUM",
    "SUB",
    "JNZ",
    "SLEEP",
    "WAIT",
    "SIGNAL",
    "MOV_IN",
    "MOV_OUT",
    "F_OPEN",
    "F_CLOSE",
    "F_SEEK",
    "F_READ",
    "F_WRITE",
    "F_TRUNCATE",
    "EXIT",
};

t_register string_a_registro(const char* string);
void solicitar_marco_a_memoria(int nro_pagina, int pid);
void connections();
void listen_kernel_dispatch_and_interrupt();
void* listen_kernel(void* socket);
void instruction_cycle();
t_instruction* get_instruccion(int socket, int id, unsigned int program_counter);
void execute(t_instruction* instruccion, t_contexto* contexto);
void handle_pf(t_contexto* contexto, uint32_t dir_logica);
void free_memory();
void solicitar_instruccion_memoria(int socket, int id, unsigned int program_counter);
uint32_t traducir_dir_logica_a_fisica(uint32_t dir_logica, t_contexto* contexto);
void solicitar_marco_a_memoria(int nro_pagina, int pid);
t_register string_a_registro(const char* string);
t_instruction* deserializar_instruccion(void* buffer_instruccion, int pid);

#endif /* CPU_H_ */
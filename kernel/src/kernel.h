#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <shared/socket.h>
#include <shared/serialization.h>
#include <shared/setup.h>
#include <shared/structures.h>
#include <semaphore.h>
#include <readline/readline.h>
#include <readline/history.h>

t_list* estados;

t_log* logger;
t_config* kernel_config;

int socket_cpu_interrupt;
int socket_cpu_dispatch;
int socket_memoria;
int socket_kernel;
int socket_fileSystem;

void initialize_sockets();
void identificar_linea(char* linea);
void free_strv(char** array);
int pedir_enum_funcion(char** sublinea);
int recibir_codigo_operacion(int socket_cliente);
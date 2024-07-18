#include <commons/collections/list.h>
#include <commons/log.h>
#include "structures.h"

#ifndef __SERIALIZATION_H
#define __SERIALIZATION_H

void add_op_code(t_buffer* buffer, op_code operation_code);
void send_buffer(int socket, t_buffer* buffer);
void destroy_buffer(t_buffer* buffer);
t_buffer* crear_buffer();
void add_to_buffer(t_buffer* buffer, void* magic, unsigned int size);
void send_buffer_complete(int socket, t_buffer* buffer);
void destruir_instruccion(t_instruction* instruccion);
t_contexto* reasignar_contexto(int socket, t_log* logger);
op_code recibir_operacion(int);
t_instruction* recibir_instruccion(int);
void eliminar_paquete(t_paquete* paquete);
void* serializar_paquete(t_paquete* paquete, int bytes);

void enviar_paquete(t_paquete* paquete, int socket_cliente);
void recibir_contexto_ejecucion(t_pcb* pcb, int socket_cpu_plani);
void enviar_contexto_ejecucion(t_pcb* pcb, int socket_cpu_plani);

void devolver_contexto(t_contexto* contexto, op_code operacion,int socket);
void devolver_contexto_con_int(t_contexto* contexto, op_code operacion, char* valor,int socket);
void devolver_contexto_con_string(t_contexto* contexto, op_code operacion, char* valor,int socket);

t_paquete* crear_paquete_codigo_operacion(op_code motivo);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

int recibir_codigo_operacion(int socket_cliente);
int redondear_hacia_arriba(double numero);

t_contexto* recibir_contexto(int socket);
void enviar_contexto(t_contexto* contexto, int socket);
void armar_y_enviar_contexto(t_pcb* pcb, int socket);

void send_op_code(int socket, op_code code);
int recibir_codigo_operacion(int socket);
int validar_codigo_recibido(int socket, op_code codigo_esperado, t_log* logger);

int contar_bloques(uint32_t* array);
int contar_enteros(int* array);
void free_strv(char** array);

#endif
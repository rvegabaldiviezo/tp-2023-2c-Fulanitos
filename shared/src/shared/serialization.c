#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <netdb.h>
#include <errno.h>
#include "serialization.h"
#include "structures.h"
#include <errno.h>
#include <unistd.h>

/////////////GENERALES//////////////////////////////////////

void send_op_code(int socket, op_code code)
{
    send(socket, &code, sizeof(op_code), 0);
}

int recibir_codigo_operacion(int socket)
{
	op_code codigo;
	if(recv(socket, &codigo, sizeof(op_code), MSG_WAITALL) > 0) return codigo;
	else { close(socket); return -1; }
}

int validar_codigo_recibido(int socket, op_code codigo_esperado, t_log* logger)
{
    op_code codigo = recibir_codigo_operacion(socket);
    if(codigo == codigo_esperado) return codigo;
    else { log_error(logger, "El código recibido [%d] no es el esperado [%d].", codigo, codigo_esperado); return -1; }
}

t_contexto* recibir_contexto(int socket)
{
    t_contexto* contexto = malloc(sizeof(t_contexto));
	recv(socket, &contexto->id, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &contexto->pc, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &contexto->registros[0], sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &contexto->registros[1], sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &contexto->registros[2], sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &contexto->registros[3], sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &contexto->interrupcion, sizeof(t_int), MSG_WAITALL);
    return contexto;
}

t_contexto* reasignar_contexto(int socket, t_log* logger)
{
    validar_codigo_recibido(socket, CONTEXTO, logger);	
    t_contexto* contexto = malloc(sizeof(t_contexto));
	recv(socket, &contexto->id, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &contexto->pc, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &contexto->registros[0], sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &contexto->registros[1], sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &contexto->registros[2], sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &contexto->registros[3], sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &contexto->interrupcion, sizeof(t_int), MSG_WAITALL);
    return contexto;
}

void enviar_contexto(t_contexto* contexto, int socket)
{
    t_paquete* paquete = crear_paquete_codigo_operacion(CONTEXTO);
    add_to_buffer(paquete->buffer, &contexto->id, sizeof(uint32_t));
    add_to_buffer(paquete->buffer, &contexto->pc, sizeof(uint32_t));
    add_to_buffer(paquete->buffer, &contexto->registros[0], sizeof(uint32_t));
    add_to_buffer(paquete->buffer, &contexto->registros[1], sizeof(uint32_t));
    add_to_buffer(paquete->buffer, &contexto->registros[2], sizeof(uint32_t));
    add_to_buffer(paquete->buffer, &contexto->registros[3], sizeof(uint32_t));
    add_to_buffer(paquete->buffer, &contexto->interrupcion, sizeof(t_int));
    enviar_paquete(paquete, socket);

    free(contexto);
}

void destruir_instruccion(t_instruction* instruccion)
{
    for(int j = 0; j < list_size(instruccion->parameters); j++)
    {
        char* parametro = list_get(instruccion->parameters, j);
        free(parametro);
    }
    free(instruccion);
}

void armar_y_enviar_contexto(t_pcb* pcb, int socket) // PCB a contexto
{
    t_contexto* contexto = malloc(sizeof(t_contexto));
    contexto->id = pcb->id;
    contexto->pc = pcb->program_counter;
    contexto->registros[0] = pcb->registros[0];
    contexto->registros[1] = pcb->registros[1];
    contexto->registros[2] = pcb->registros[2];
    contexto->registros[3] = pcb->registros[3];
    contexto->interrupcion = pcb->interrupcion;
    enviar_contexto(contexto, socket);
}

int count_elements(char** array) 
{
    int count = 0;
    while (array[count] != NULL) count++;
    return count;
}

int contar_bloques(uint32_t* array) 
{
    size_t tamanio = sizeof(array);
    if(tamanio != 0)
    {
        int n = sizeof(array) / sizeof(array[0]);
        return n;
    }
    else return 0;
}

void destroy_buffer(t_buffer* buffer)
{
    free(buffer->stream);
    free(buffer);
}

t_buffer* crear_buffer() 
{
    t_buffer* buffer = malloc(sizeof(t_buffer));
    buffer->size = 0;
    buffer->stream = NULL;
    return buffer;
}

void add_to_buffer(t_buffer* buffer, void* magic, unsigned int size)
{
    if (buffer->stream == NULL) {
        buffer->stream = malloc(buffer->size + size);
    } else {
        buffer->stream = realloc(buffer->stream, buffer->size + size);
    }

    if (buffer->stream != NULL) {
        memcpy(buffer->stream + buffer->size, magic, size);
        buffer->size += size;
    }
}

void send_buffer(int socket, t_buffer* buffer)
{
    send(socket, buffer->stream, buffer->size, 0);
    destroy_buffer(buffer);
}

void add_op_code(t_buffer* buffer, op_code operation_code)
{
    add_to_buffer(buffer, &operation_code, sizeof(int));
}

op_code recibir_operacion(int socket_cliente) {
    op_code codigo;

    int bytes_recibidos;
    while ((bytes_recibidos = recv(socket_cliente, &codigo, sizeof(op_code), MSG_WAITALL)) != sizeof(op_code)) {
        if (bytes_recibidos == -1) {
        	printf("ERROR EN RECIBIR OPERACION: %s", strerror(errno));
        }
    }

    return codigo;
}

t_instruction* recibir_instruccion(int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->buffer = malloc(sizeof(t_buffer));

	//Recibo tamanio buffer
	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	//Recibo contenido buffer
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

	int pos = 0;
	int tam_param;
    int tamanio;

	t_instruction* instruccion_temp = malloc(sizeof(t_instruction));
    instruccion_temp->parameters = list_create();

	//Recibo codigo primera instruccion
	memcpy(&tamanio, paquete->buffer->stream + pos, sizeof(int));
    pos+=sizeof(int);
    t_instruction_type operacion;
    memcpy(&operacion, paquete->buffer->stream + pos, sizeof(int));
    pos+=tamanio;
    instruccion_temp->operation = operacion;

    memcpy(&tamanio, paquete->buffer->stream + pos, sizeof(int));
    pos+=sizeof(int);
    int cantidad_parametros;
    memcpy(&cantidad_parametros, paquete->buffer->stream + pos, sizeof(int));
    pos+=tamanio;

	//Recibo los parametros
    for(int i=0; i<cantidad_parametros; i++){
    memcpy(&tamanio, paquete->buffer->stream + pos, sizeof(int));
    pos+=sizeof(int);
    char* parametro = malloc(tamanio);
    memcpy(parametro, paquete->buffer->stream + pos, tamanio);
    pos+=tamanio;
    list_add(instruccion_temp->parameters, parametro);
    }
	return instruccion_temp;
}

t_paquete* crear_paquete_codigo_operacion(op_code motivo) 
{
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = motivo;
    paquete->buffer = crear_buffer();
    return paquete;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(op_code));
	desplazamiento+= sizeof(op_code);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

    destroy_buffer(paquete->buffer);
    free(paquete);

	return magic;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = sizeof(op_code) + paquete->buffer->size;
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

int redondear_hacia_arriba(double numero) 
{
	int nro_entero = numero;
	if(nro_entero < numero)
		return nro_entero + 1;
	else
		return numero;
}

void free_strv(char** array)
{
    if (array != NULL) {
        for (int i = 0; array[i] != NULL; i++) {
            free(array[i]);
        }
        free(array);
    }
}

void send_buffer_complete(int socket, t_buffer* buffer)
{
    t_buffer* bufferaux = crear_buffer(); 
    add_to_buffer(bufferaux, &buffer->size, sizeof(int));
    add_to_buffer(bufferaux, buffer->stream, buffer->size);
    send(socket, bufferaux->stream, bufferaux->size, 0); 
    destroy_buffer(bufferaux);
}

////////////////KERNEL//////////////////////////////////////
/* 
void recibir_contexto_ejecucion(t_pcb* pcb, int socket){
	int pc;
    int desp = 0;
	recv(socket, &pc, sizeof(int), MSG_WAITALL);
    desp+=sizeof(int);

	uint32_t registros[4];
	recv(socket, &registros[0], sizeof(uint32_t), MSG_WAITALL);
    desp+=sizeof(uint32_t);
    recv(socket, &registros[1], sizeof(uint32_t), MSG_WAITALL);
    desp+=sizeof(uint32_t);
    recv(socket, &registros[2], sizeof(uint32_t), MSG_WAITALL);
    desp+=sizeof(uint32_t);
    recv(socket, &registros[3], sizeof(uint32_t), MSG_WAITALL);
    desp+=sizeof(uint32_t);


	pcb->program_counter = pc;
    pcb->registros[0] = registros[0];
    pcb->registros[1] = registros[1];
    pcb->registros[2] = registros[2];
    pcb->registros[3] = registros[3];
    
    /*
	int cantidad_segmentos;
	recv(socket_cpu_plani, &cantidad_segmentos, sizeof(int), MSG_WAITALL);    
	t_list* lista_segmentos = list_create();
	for(int i = 0; i < cantidad_segmentos; i++){
		t_segmento* segmento = malloc(sizeof(t_segmento));

		recv(socket_cpu_plani, &(segmento->id), sizeof(int), MSG_WAITALL);
		recv(socket_cpu_plani, &(segmento->direccion_base), sizeof(int), MSG_WAITALL);
		recv(socket_cpu_plani, &(segmento->tamanio), sizeof(int), MSG_WAITALL);

		list_add(lista_segmentos, segmento);
	}
	list_destroy_and_destroy_elements(pcb->tabla_segmentos, free);
	pcb->tabla_segmentos = lista_segmentos;
    
    
    } 
*/


/* void enviar_contexto_ejecucion(t_pcb* pcb, int socket){

	//t_buffer* lista_serializada = serializar_lista(pcb->instrucciones);
	//int cant_segmentos = list_size(pcb->tabla_segmentos);
	//int tamanio_segmentos = cant_segmentos*3*sizeof(int);
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = CONTEXTO;
	//paquete->buffer = lista_serializada;

	void* a_enviar = malloc(
			+ sizeof(op_code)
			+ sizeof(int)
			+ sizeof(int)
			+ sizeof(char*)*4);
			//+ sizeof(int)
			//+ sizeof(int));
	int desp = 0;

	//Copio codigo de operacion
	memcpy(a_enviar + desp, &(paquete->codigo_operacion), sizeof(op_code));
	desp += sizeof(op_code);

	//Copio pid
	memcpy(a_enviar + desp, &(pcb->id), sizeof(int));
	desp += sizeof(int);

	//Copio program_counter
	memcpy(a_enviar + desp, &(pcb->program_counter), sizeof(int));
	desp += sizeof(int);

	//Copio registros
	memcpy(a_enviar + desp, &(pcb->registros[0]), sizeof(char*));
	desp += sizeof(char*);
    memcpy(a_enviar + desp, &(pcb->registros[1]), sizeof(char*));
    desp += sizeof(char*);
    memcpy(a_enviar + desp, &(pcb->registros[2]), sizeof(char*));
    desp += sizeof(char*);
    memcpy(a_enviar + desp, &(pcb->registros[3]), sizeof(char*));
    desp += sizeof(char*);


    /*
	//Copio tamanio del buffer
	memcpy(a_enviar + desp, &(paquete->buffer->size), sizeof(int));
	desp += sizeof(int);

	//Copio buffer completo
	memcpy(a_enviar + desp, paquete->buffer->stream, paquete->buffer->size);
	desp += paquete->buffer->size;
    */
    /*
	//Copio cantidad segmentos
	memcpy(a_enviar + desp, &(cant_segmentos), sizeof(int));
	desp += sizeof(int);

	t_list* lista = pcb->tabla_segmentos;
	for(int i = 0; i<cant_segmentos; i++){
		//Copio cantidad segmentos
		t_segmento* seg = (t_segmento*)list_get(lista, i);
		memcpy(a_enviar + desp, &(seg->id), sizeof(int));
		desp += sizeof(int);
		memcpy(a_enviar + desp, &(seg->direccion_base), sizeof(int));
		desp += sizeof(int);
		memcpy(a_enviar + desp, &(seg->tamanio), sizeof(int));
		desp += sizeof(int);
	}
    
    
	//Envio paquete
	send(socket, a_enviar, desp, 0);

	//free(lista_serializada->stream);
	//free(lista_serializada);
	free(a_enviar);
	free(paquete);
} 
*/

//////////////////CPU///////////////////////////

/* void recibir_contexto(t_contexto* contexto, int socket){

	//Recibo pid
	int pid;
	recv(socket, &pid, sizeof(int), MSG_WAITALL);

	//Recibo program_counter
	int program_counter;
	recv(socket, &program_counter, sizeof(int), MSG_WAITALL);

	//Recibo registros
	char* registros[4];

	recv(socket, &registros[0], sizeof(char*), MSG_WAITALL);
    recv(socket, &registros[1], sizeof(char*), MSG_WAITALL);
    recv(socket, &registros[2], sizeof(char*), MSG_WAITALL);
    recv(socket, &registros[3], sizeof(char*), MSG_WAITALL);

    
	contexto->pc= program_counter;
	contexto->id = pid;
    contexto->registros[0] = registros[0];
    contexto->registros[1] = registros[1];
    contexto->registros[2] = registros[2];
    contexto->registros[3] = registros[3];
    

} 

void devolver_contexto(t_contexto* contexto, op_code operacion,int socket){

	void* a_enviar = malloc(sizeof(op_code) + sizeof(int)+sizeof(char*)*4);
	int desp = 0;

	//Copio codigo de operacion
	memcpy(a_enviar + desp, &operacion, sizeof(op_code));
	desp += sizeof(op_code);

	//Copio program_counter
	memcpy(a_enviar + desp, &(contexto->pc), sizeof(int));
	desp += sizeof(int);

	//Copio registros
	memcpy(a_enviar + desp, &(contexto->registros[0]), sizeof(char*));
	desp += sizeof(char*);
    memcpy(a_enviar + desp, &(contexto->registros[1]), sizeof(char*));
    desp += sizeof(char*);
    memcpy(a_enviar + desp, &(contexto->registros[2]), sizeof(char*));
    desp += sizeof(char*);
    memcpy(a_enviar + desp, &(contexto->registros[3]), sizeof(char*));
    desp += sizeof(char*);


	//Envio paquete
	send(socket, a_enviar, sizeof(op_code)+ sizeof(int)+sizeof(char*)*4 , 0);

	free(a_enviar);
}

void devolver_contexto_con_int(t_contexto* contexto, op_code operacion, char* valor,int socket){
	void* a_enviar = malloc(sizeof(op_code) + sizeof(int)+sizeof(char*)*4);
	int desp = 0;
    int tiempo = atoi(valor);

	//Copio codigo de operacion
	memcpy(a_enviar + desp, &operacion, sizeof(op_code));
	desp += sizeof(op_code);

	//Copio program_counter
	memcpy(a_enviar + desp, &(contexto->pc), sizeof(int));
	desp += sizeof(int);

	//Copio registros
	memcpy(a_enviar + desp, &(contexto->registros[0]), sizeof(char*));
	desp += sizeof(char*);
    memcpy(a_enviar + desp, &(contexto->registros[1]), sizeof(char*));
    desp += sizeof(char*);
    memcpy(a_enviar + desp, &(contexto->registros[2]), sizeof(char*));
    desp += sizeof(char*);
    memcpy(a_enviar + desp, &(contexto->registros[3]), sizeof(char*));
    desp += sizeof(char*);


	//Copio valor
	memcpy(a_enviar + desp, &tiempo, sizeof(int));
	desp += sizeof(int);

	//Envio paquete
	send(socket, a_enviar, sizeof(op_code) +sizeof(int)*2+sizeof(char*)*4, 0);

	free(a_enviar);

    printf("Se envio el tiempo de ejecucion\n");
}


void devolver_contexto_con_string(t_contexto* contexto, op_code operacion, char* valor, int socket) {
    int tamanio_param = strlen(valor);

    // Calcular el tamaño total del paquete
    size_t tamano_paquete = sizeof(op_code) + sizeof(int) * 2 + sizeof(char*) * 4 + tamanio_param;

    // Asignar memoria para el paquete
    void* a_enviar = malloc(tamano_paquete);
    if (a_enviar == NULL) {
        perror("Error al asignar memoria para el paquete");
        return;
    }

    // Inicializar el desplazamiento
    size_t desp = 0;

    // Copiar el código de operación
    memcpy(a_enviar + desp, &operacion, sizeof(op_code));
    desp += sizeof(op_code);

    // Copiar el program_counter
    memcpy(a_enviar + desp, &(contexto->pc), sizeof(int));
    desp += sizeof(int);

    // Copiar los registros
    for (int i = 0; i < 4; i++) {
        memcpy(a_enviar + desp, &(contexto->registros[i]), sizeof(char*));
        desp += sizeof(char*);
    }

    // Copiar el tamaño de la cadena
    memcpy(a_enviar + desp, &tamanio_param, sizeof(int));
    desp += sizeof(int);

    // Copiar la cadena
    memcpy(a_enviar + desp, valor, tamanio_param);
    desp += tamanio_param;

    // Enviar el paquete
    ssize_t bytes_enviados = send(socket, a_enviar, tamano_paquete, 0);
    if (bytes_enviados == -1) {
        printf("Error al enviar el paquete");
    } else if (bytes_enviados < tamano_paquete) {
        fprintf(stderr, "No se enviaron todos los bytes esperados. Se enviaron %zd bytes de %zu\n", bytes_enviados, tamano_paquete);
    }

    // Liberar memoria
    free(a_enviar);
}
*/
/////////////////////////////////////////////////////////////
//PROBANDO FUNCIONES/////////////////////////////////////////
/////////////////////////////////////////////////////////////

/*

void* recibir_buffer(int socket_cliente, int* size) {
    void * buffer;

    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);

    return buffer;
}


t_contexto* recibir_contexto_nuevo(int socket)
{   

    t_contexto* contexto = malloc(sizeof(t_contexto));

    //contexto->instrucciones = list_create();

    int size;
    int desplazamiento = 0;
    void * buffer;
    int tamanio;

    buffer = recibir_buffer(socket, &size);


    memcpy(&(contexto->id), buffer + desplazamiento, sizeof(int));
    desplazamiento+=sizeof(int);
    memcpy(&(contexto->pc), buffer + desplazamiento, sizeof(int));
    desplazamiento+=sizeof(int);
    memcpy(&(contexto->registros[0]), buffer + desplazamiento, sizeof(int));
    desplazamiento+=sizeof(int);
    memcpy(&(contexto->registros[1]), buffer + desplazamiento, sizeof(int));
    desplazamiento+=sizeof(int);
    memcpy(&(contexto->registros[2]), buffer + desplazamiento, sizeof(int));
    desplazamiento+=sizeof(int);
    memcpy(&(contexto->registros[3]), buffer + desplazamiento, sizeof(int));
    desplazamiento+=sizeof(int);


    free(buffer);

    return contexto;

}


////////////////////////////////////////////////////

void crear_buffer(t_paquete* paquete) 
{
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
}





void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio) {
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

    paquete->buffer->size += tamanio + sizeof(int);
}



void eliminar_paquete(t_paquete* paquete) {
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}



    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento+= sizeof(int);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento+= sizeof(int);
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento+= paquete->buffer->size;

    return magic;
}

void enviar_contexto_nuevo(int socket, t_contexto* contexto,op_code motivo)
{   
    t_paquete* paquete = crear_paquete_codigo_operacion(motivo);

    paquete->buffer->size = sizeof(int) + sizeof(int) * 4 + sizeof(int);

    void* stream = malloc(paquete->buffer->size);
    int offset = 0;

    memcpy(stream + offset, &contexto->id, sizeof(int));
    memcpy(stream + offset, &contexto->id, sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, &contexto->pc, sizeof(int));
    memcpy(stream + offset, &contexto->pc, sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, &contexto->registros[0], sizeof(int));
    memcpy(stream + offset, &contexto->registros[0], sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, &contexto->registros[1], sizeof(int));
    memcpy(stream + offset, &contexto->registros[1], sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, &contexto->registros[2], sizeof(int));
    memcpy(stream + offset, &contexto->registros[2], sizeof(int));
    offset += sizeof(int);
    memcpy(stream + offset, &contexto->registros[3], sizeof(int));
    memcpy(stream + offset, &contexto->registros[3], sizeof(int));
    offset += sizeof(int);

    paquete->buffer->stream = stream;

    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}  


void add_a_paquete(t_paquete* paquete, t_list* lista) //Anda dentro de todo
{   

    int cantidad_instrucciones = list_size(lista);
    agregar_a_paquete(paquete, &cantidad_instrucciones, sizeof(int));

    printf("Size de la cantidad de instrucciones: %d\n", paquete->buffer->size);

    for(int i = 0; i < cantidad_instrucciones; i++)
    {
        t_instruction* instruccion = list_get(lista, i);
        int operacion = instruccion->operation;
        agregar_a_paquete(paquete,&operacion, sizeof(int));
        int cantidad_parametros = list_size(instruccion->parameters);
        agregar_a_paquete(paquete, &cantidad_parametros, sizeof(int));
        for(int j = 0; j < cantidad_parametros; j++)
        {
            char* parametro = list_get(instruccion->parameters, j);
            int largo_parametro = strlen(parametro) + 1;
            agregar_a_paquete(paquete, parametro, largo_parametro);
        }

        free(instruccion);
    }

}


void enviar_mensaje(int socket_server, char* mensaje) {
    t_paquete* paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = INSTRUCCIONES;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2*sizeof(int);

    void* a_enviar = serializar_paquete(paquete, bytes);

    send(socket_server, a_enviar, bytes, 0);
    printf("Mensaje enviado");
    free(a_enviar);
    eliminar_paquete(paquete);
}

char* recibir_mensaje(int socket_cliente) {
    int size;
    char* buffer = recibir_buffer(socket_cliente, &size);
    printf("Mensaje recibido: %s", buffer);
    //free(buffer);

    return buffer;
}
*/
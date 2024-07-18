#include <cpu.h>
sem_t debug;
bool contexto_devuelto = false; 
bool esperando_devolucion = false;
#define IP_CONFIG_PATH "/home/utnso/tp-2023-2c-Fulanitos/config/ip.config"

int main(int argc, char **argv) 
{
	initialize_setup(argv[1], "cpu", &logger, &cpu_config);
	connections(); 
	sem_init(&debug, 0, 0);
	interrupt_pid = -1;
	interrupt_type = 0;
	listen_kernel_dispatch_and_interrupt();
	free_memory();   
	return EXIT_SUCCESS;
}

void connections()
{
	char* path = "/home/utnso/tp-2023-2c-Fulanitos/config/ip.config";
	socket_memoria = start_client_module("MEMORIA",path);
	log_info(logger, "Conexion con servidor MEMORIA en socket %d", socket_memoria);

	socket_cpu_interrupt = start_server_module("CPU_INTERRUPT",path);
	log_info(logger, "Creacion de servidor CPU INTERRUPT en socket %d", socket_cpu_interrupt);

	socket_cpu_dispatch = start_server_module("CPU_DISPATCH",path);
	log_info(logger, "Creacion de servidor CPU DISPATCH en socket %d", socket_cpu_dispatch);
	
	log_info(logger, "Esperando a Kernel...");
 
	socket_kernel_interrupt = accept(socket_cpu_interrupt, NULL, NULL); 
	log_info(logger, "Conexion INTERRUPT con cliente KERNEL en socket %i", socket_kernel_interrupt);

	socket_kernel_dispatch = accept(socket_cpu_dispatch, NULL, NULL); 
	log_info(logger, "Conexion DISPATCH con cliente KERNEL en socket %i", socket_kernel_dispatch);
	
	send_op_code(socket_memoria, HANDSHAKE);
	recv(socket_memoria, &tam_pagina, sizeof(int), MSG_WAITALL);
}

void listen_kernel_dispatch_and_interrupt()
{
	if(0 != pthread_create(&thread_interrupt, NULL, listen_kernel, (void*)&socket_kernel_interrupt))
	{
		log_info(logger, "No se pudo crear el hilo interrupt.");
		exit(EXIT_FAILURE);
	}
	log_info(logger, "Hilo interrupt creado.");		

	listen_kernel((void*)&socket_kernel_dispatch);
}

void* listen_kernel(void* socket_kernel)
{
	int socket = *(int*) socket_kernel;
	while(true) 
	{
		//log_debug(logger, "Esperando mensaje de kernel");
		op_code cod_op = recibir_codigo_operacion(socket);
	 	switch (cod_op) 
		{		
			case CONTEXTO:
			{
				log_debug(logger, "Contexto recibido.");
				instruction_cycle();  
				break;
			}
			case INTERRUPCION:
			{
				recv(socket, &interrupt_type, sizeof(t_int), MSG_WAITALL);
				recv(socket, &interrupt_pid, sizeof(int), MSG_WAITALL);
				log_debug(logger, "Interrupción %d recibida en proceso %d.", interrupt_type, interrupt_pid);
				break;
			}
			default:
			{
				if(cod_op == -1)
				{
					log_trace(logger, "Error critico. Finalizando ejecucion.");
					exit(EXIT_FAILURE);
				}
				else
				{
					log_trace(logger, "Codigo desconocido.");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	return NULL;
}

void instruction_cycle()
{	
	t_contexto* contexto = recibir_contexto(socket_kernel_dispatch); 
	t_instruction* instruccion = NULL;
	t_instruction_type inst = -1;
	bool interrupcion = false;
	contexto_devuelto = false;
	esperando_devolucion = false;

	do
	{		
		if (esperando_devolucion) 
		{
			log_debug(logger, "Reasignando contexto");
			contexto = reasignar_contexto(socket_kernel_dispatch, logger);			
			esperando_devolucion = false;
		}

		log_info(logger, "PID: %d - FETCH - Program Counter: %d.", contexto->id, contexto->pc);
		instruccion = get_instruccion(socket_memoria, contexto->id, contexto->pc); 
		execute(instruccion, contexto);
		log_debug(logger, "Ejecución terminada");
				
		if(contexto->id == interrupt_pid) // TODO: en que caso se descartaria la interrupcion?
		{ 
			interrupt_pid = -1;
			interrupt_type = 0;
			interrupcion = true;

			if(esperando_devolucion) contexto = NULL;

			if(contexto != NULL && !contexto_devuelto)
			{
				contexto->interrupcion = interrupt_type; 		
				log_debug(logger, "Desalojo %d", contexto->id);
				enviar_contexto(contexto, socket_kernel_dispatch);
				send_op_code(socket_kernel_dispatch, DESALOJO);
			}
		}

		inst = instruccion->operation; 
		if(inst == EXIT) log_debug(logger, "EXIT");
		destruir_instruccion(instruccion);
	}
	while(inst != EXIT && interrupcion != true && contexto_devuelto == false);
	log_debug(logger, "Vuelvo a kernel");
}

void execute(t_instruction* instruction, t_contexto* contexto)
{ 
	char* nombre_archivo = NULL;
		size_t largo_nombre = 0;
		uint32_t tamanio_a_truncar = 0;
		uint32_t dir_logica = 0;
		uint32_t dir_fisica = 0;
		uint32_t valor = 0;
		uint32_t puntero_archivo = 0;
		t_register registro_destino;
		t_register registro_origen;
		char* modo_apertura = NULL;
		char* parametro = NULL;
		char* parametro2 = NULL;
		char* recurso = NULL;
		contexto->pc++; // TODO: no hay problema con incrementarlo antes de ejecutar instruccion?
	switch (instruction->operation) 
	{
		case SLEEP: 
		{
			parametro = list_get(instruction->parameters, 0);
			int segundos = atoi(parametro); 
			
			enviar_contexto(contexto, socket_kernel_dispatch);

			t_paquete* paquete = crear_paquete_codigo_operacion(SLEEP_BLOQUEANTE);
			add_to_buffer(paquete->buffer, &segundos, sizeof(int));
			enviar_paquete(paquete, socket_kernel_dispatch);

			contexto = NULL;
			contexto_devuelto = true; 
			break;
		}
		case WAIT: 
		{
			recurso = list_get(instruction->parameters, 0);
			largo_nombre = strlen(recurso) + 1;

			enviar_contexto(contexto, socket_kernel_dispatch);
			t_paquete* paquete = crear_paquete_codigo_operacion(SOLICITAR_WAIT);
			add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
			add_to_buffer(paquete->buffer, recurso, largo_nombre);
			enviar_paquete(paquete, socket_kernel_dispatch);
			
			int codigo = recibir_codigo_operacion(socket_kernel_dispatch);
			if(codigo == OK) esperando_devolucion = true;
			else contexto_devuelto = true;
			break;
		}
		case SIGNAL: 
		{
			recurso = list_get(instruction->parameters, 0);
			largo_nombre = strlen(recurso) + 1;

			enviar_contexto(contexto, socket_kernel_dispatch);
			t_paquete* paquete = crear_paquete_codigo_operacion(SOLICITAR_SIGNAL);
			add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
			add_to_buffer(paquete->buffer, recurso, largo_nombre);
			enviar_paquete(paquete, socket_kernel_dispatch);
		
			int codigo = recibir_codigo_operacion(socket_kernel_dispatch);
			if(codigo == OK) esperando_devolucion = true;
			else contexto_devuelto = true;
			break;
		}
		case SET:
		{
			parametro = list_get(instruction->parameters, 0);
			parametro2 = list_get(instruction->parameters, 1);
			registro_destino = string_a_registro(parametro);
			valor = atoi(parametro2);
			contexto->registros[registro_destino] = valor;
			break;
		}
		case SUM: 
		{
			parametro = list_get(instruction->parameters, 0);
			parametro2 = list_get(instruction->parameters, 1);
			registro_destino = string_a_registro(parametro);
			registro_origen = string_a_registro(parametro2);
			contexto->registros[registro_destino] = contexto->registros[registro_origen] + contexto->registros[registro_destino];
			break;
		}
		case SUB:
		{
			parametro = list_get(instruction->parameters, 0);
			parametro2 = list_get(instruction->parameters, 1);
			registro_destino = string_a_registro(parametro);
			registro_origen = string_a_registro(parametro2);
			contexto->registros[registro_destino] = contexto->registros[registro_destino] - contexto->registros[registro_origen];
			break;
		}
		case JNZ:
		{
			parametro = list_get(instruction->parameters, 0);
			registro_destino = string_a_registro(parametro);
			if(contexto->registros[registro_destino] != 0)
			{
				parametro2 = list_get(instruction->parameters, 1);
				uint32_t nuevo_pc = atoi(parametro2);
				contexto->pc = nuevo_pc; 
			}
			break;
		}
		case MOV_IN:
		{
			parametro2 = list_get(instruction->parameters, 1);
			dir_logica = atoi(parametro2);
			dir_fisica = traducir_dir_logica_a_fisica(dir_logica, contexto);
			
			if(dir_fisica != -1)
			{
				parametro = list_get(instruction->parameters, 0);
				registro_destino = string_a_registro(parametro);

				t_paquete* paquete = crear_paquete_codigo_operacion(READ);
				add_to_buffer(paquete->buffer, &contexto->id, sizeof(int));
				add_to_buffer(paquete->buffer, &dir_fisica, sizeof(uint32_t));				
				int nro_pagina = floor(dir_logica / tam_pagina);
				add_to_buffer(paquete->buffer, &nro_pagina, sizeof(int));	
				enviar_paquete(paquete, socket_memoria);
				
				void* contenido = malloc(tam_pagina);
				recv(socket_memoria, contenido, sizeof(uint32_t), MSG_WAITALL);

				uint32_t* p = (uint32_t*)contenido;
				uint32_t valor = *p;
				contexto->registros[registro_destino] = valor;

				log_info(logger, "PID: %d - Acción: LEER - Dirección Física: %d - Valor: %d.", contexto->id, dir_fisica, valor);				
			}
			else 
			{				
				handle_pf(contexto, dir_logica);
				contexto_devuelto = true;
			}					
			break;
		}
		case MOV_OUT:
		{
			parametro = list_get(instruction->parameters, 0);
			dir_logica = atoi(parametro);
			dir_fisica = traducir_dir_logica_a_fisica(dir_logica, contexto);
			
			if(dir_fisica != -1)
			{
				parametro2 = list_get(instruction->parameters, 1);
				registro_origen = string_a_registro(parametro2);
				valor = contexto->registros[registro_origen];

				t_paquete* paquete = crear_paquete_codigo_operacion(WRITE);
				add_to_buffer(paquete->buffer, &contexto->id, sizeof(int));
				add_to_buffer(paquete->buffer, &dir_fisica, sizeof(uint32_t));
				add_to_buffer(paquete->buffer, &valor, sizeof(uint32_t));
				int nro_pagina = floor(dir_logica / tam_pagina);
				add_to_buffer(paquete->buffer, &nro_pagina, sizeof(int));
				enviar_paquete(paquete, socket_memoria);

				validar_codigo_recibido(socket_memoria, OK, logger);
				log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d.", contexto->id, dir_fisica, valor);
			}
			else 
			{
				log_debug(logger, "Page fault");
				handle_pf(contexto, dir_logica);
				contexto_devuelto = true;
			}
			break;
		}
		case F_OPEN:
		{
			nombre_archivo = list_get(instruction->parameters, 0);
			modo_apertura = list_get(instruction->parameters, 1);

			enviar_contexto(contexto, socket_kernel_dispatch);
			t_paquete* paquete = crear_paquete_codigo_operacion(OPEN);
			largo_nombre = (strlen(nombre_archivo) + 1) * sizeof(char);
			add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
			add_to_buffer(paquete->buffer, nombre_archivo, largo_nombre);
			add_to_buffer(paquete->buffer, &modo_apertura[0], sizeof(char));
			enviar_paquete(paquete, socket_kernel_dispatch);

			op_code respuesta = recibir_codigo_operacion(socket_kernel_dispatch);
			if(respuesta == OK)
			{					
				esperando_devolucion = true;
			}
			else if(respuesta == BLOQUEADO)
			{
				contexto_devuelto = true;
			}

			break;
		}
		case F_CLOSE:
		{
			nombre_archivo = list_get(instruction->parameters, 0);
			enviar_contexto(contexto, socket_kernel_dispatch);
			t_paquete* paquete = crear_paquete_codigo_operacion(CLOSE);
			largo_nombre = (strlen(nombre_archivo) + 1) * sizeof(char);
			add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
			add_to_buffer(paquete->buffer, nombre_archivo, largo_nombre);
			enviar_paquete(paquete, socket_kernel_dispatch);

			esperando_devolucion = true;
			break;
		}
		case F_SEEK:
		{
			nombre_archivo = list_get(instruction->parameters, 0);
			parametro2 = list_get(instruction->parameters, 1);
			puntero_archivo = atoi(parametro2);

			enviar_contexto(contexto, socket_kernel_dispatch);
			t_paquete* paquete = crear_paquete_codigo_operacion(SEEK);
			largo_nombre = (strlen(nombre_archivo) + 1) * sizeof(char);

			add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
			add_to_buffer(paquete->buffer, nombre_archivo, largo_nombre);
			add_to_buffer(paquete->buffer, &puntero_archivo, sizeof(uint32_t));
			enviar_paquete(paquete, socket_kernel_dispatch);

			esperando_devolucion = true;
			break;
		}
		case F_READ:
		{ 
			parametro = list_get(instruction->parameters, 1);
			dir_logica = atoi(parametro); 
			dir_fisica = traducir_dir_logica_a_fisica(dir_logica, contexto);

			if(dir_fisica != -1)
			{
				nombre_archivo = list_get(instruction->parameters, 0);
				enviar_contexto(contexto, socket_kernel_dispatch);
				t_paquete* paquete = crear_paquete_codigo_operacion(READ);
				largo_nombre = (strlen(nombre_archivo) + 1) * sizeof(char);
				add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
				add_to_buffer(paquete->buffer, nombre_archivo, largo_nombre);
				add_to_buffer(paquete->buffer, &dir_fisica, sizeof(uint32_t));
				add_to_buffer(paquete->buffer, &tam_pagina, sizeof(uint32_t));
				int nro_pagina = floor(dir_logica / tam_pagina);
				add_to_buffer(paquete->buffer, &nro_pagina, sizeof(int));
				enviar_paquete(paquete, socket_kernel_dispatch);

				esperando_devolucion = true;
			}
			else 
			{
				contexto_devuelto = true;
				handle_pf(contexto, dir_logica); // TODO: primero pf o leer archivo?
			}

			break;
		}
		case F_WRITE: 
		{
			parametro = list_get(instruction->parameters, 1);
			dir_logica = atoi(parametro); // escribir desde puntero en kernel
			dir_fisica = traducir_dir_logica_a_fisica(dir_logica, contexto);

			if(dir_fisica != -1)
			{
				nombre_archivo = list_get(instruction->parameters, 0);
				enviar_contexto(contexto, socket_kernel_dispatch);
				t_paquete* paquete = crear_paquete_codigo_operacion(WRITE);
				largo_nombre = (strlen(nombre_archivo) + 1) * sizeof(char);
				add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
				add_to_buffer(paquete->buffer, nombre_archivo, largo_nombre);
				add_to_buffer(paquete->buffer, &dir_fisica, sizeof(uint32_t));							
				add_to_buffer(paquete->buffer, &tam_pagina, sizeof(uint32_t));
				int nro_pagina = floor(dir_logica / tam_pagina);
				add_to_buffer(paquete->buffer, &nro_pagina, sizeof(int));
				enviar_paquete(paquete, socket_kernel_dispatch);


				op_code respuesta = recibir_codigo_operacion(socket_kernel_dispatch);
				if(respuesta == OK)
				{					
					esperando_devolucion = true;
				}
				else if(respuesta == FIN_PROCESO)
				{
					contexto_devuelto = true;
				}
			}
			else 
			{
				contexto_devuelto = true;
				handle_pf(contexto, dir_logica); // TODO: primero pf o leer archivo?
			}

			break;
		}
		case F_TRUNCATE:
		{
			nombre_archivo = list_get(instruction->parameters, 0);
			parametro2 = list_get(instruction->parameters, 1);
			tamanio_a_truncar = atoi(parametro2);

			enviar_contexto(contexto, socket_kernel_dispatch);
			t_paquete* paquete = crear_paquete_codigo_operacion(TRUNCATE);
			largo_nombre = (strlen(nombre_archivo) + 1) * sizeof(char);
			add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
			add_to_buffer(paquete->buffer, nombre_archivo, largo_nombre);
			add_to_buffer(paquete->buffer, &tamanio_a_truncar, sizeof(uint32_t));
			enviar_paquete(paquete, socket_kernel_dispatch);

			esperando_devolucion = true;
			break;
		}
		case EXIT:
		{
			t_paquete* paquete = malloc(sizeof(t_paquete));
			paquete->buffer = crear_buffer();
			add_to_buffer(paquete->buffer, &contexto->id, sizeof(uint32_t));
			add_to_buffer(paquete->buffer, &contexto->pc, sizeof(uint32_t));
			add_to_buffer(paquete->buffer, &contexto->registros[0], sizeof(uint32_t));
			add_to_buffer(paquete->buffer, &contexto->registros[1], sizeof(uint32_t));
			add_to_buffer(paquete->buffer, &contexto->registros[2], sizeof(uint32_t));
			add_to_buffer(paquete->buffer, &contexto->registros[3], sizeof(uint32_t));
			add_to_buffer(paquete->buffer, &contexto->interrupcion, sizeof(t_int));

			// int bytes = sizeof(op_code) + paquete->buffer->size;
			// void* magic = malloc(bytes);
			// int desplazamiento = 0;

			// memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(op_code));
			// desplazamiento+= sizeof(op_code);
			// memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
			// desplazamiento+= paquete->buffer->size;

			send_op_code(socket_kernel_dispatch, CONTEXTO);
			send(socket_kernel_dispatch, paquete->buffer, paquete->buffer->size, 0);	

			destroy_buffer(paquete->buffer);
			free(paquete);
			free(contexto);
			//enviar_contexto(contexto, socket_kernel_dispatch);
			send_op_code(socket_kernel_dispatch, PROCESS_FINISHED);
			interrupt_pid = -1;
			break;
		}
		default: 
		{
			log_error(logger, "Código %d desconocido.", instruction->operation);
			exit(EXIT_FAILURE);
		}
	}
}

void handle_pf(t_contexto* contexto, uint32_t dir_logica)
{
	int nro_pagina = (int)floor(dir_logica / tam_pagina);

	log_info(logger, "Page Fault PID: %d - Página: %d.", contexto->id, nro_pagina);

	contexto->pc--;
	enviar_contexto(contexto, socket_kernel_dispatch);
	t_paquete* paquete = crear_paquete_codigo_operacion(PAGE_FAULT);
	add_to_buffer(paquete->buffer, &nro_pagina, sizeof(int));
	enviar_paquete(paquete, socket_kernel_dispatch);
	
	// interrupt_pid = -1; TODO: no se por que lo puse
}

void free_memory(){
	//agregar la linea para que espere al resto de los hilos, si no finaliza con  el hilo principal 
	//Hilos
	pthread_join(thread_interrupt, NULL);
	log_info(logger,"Finalizamos el Hilo llamado thread_interrupt");
	//Config
	config_destroy(cpu_config);
	log_destroy(logger);
	// liberar_conexion(cpu->kernel_dispatch);
	// liberar_conexion(cpu->kernel_interrupt);
	// liberar_conexion(cpu->servidor_dispatch);
	// liberar_conexion(cpu->servidor_interrupt);
	// liberar_conexion(cpu->memoria);
}

t_instruction* get_instruccion(int socket_cpu_memoria, int id, unsigned int program_counter)
{
	solicitar_instruccion_memoria(socket_cpu_memoria, id, program_counter);

	int size = 0;
	void* buffer = NULL;
	recv(socket_cpu_memoria, &size, sizeof(int), MSG_WAITALL);
	buffer = malloc(size);
	recv(socket_cpu_memoria, buffer, size, MSG_WAITALL);
    t_instruction* instruccion = malloc(sizeof(t_instruction));
    instruccion =  deserializar_instruccion(buffer, id);
	
	free(buffer);
	return instruccion;
} 

void solicitar_instruccion_memoria(int socket, int id, unsigned int program_counter)
{
	t_paquete* paquete = crear_paquete_codigo_operacion(INSTRUCCION);
	add_to_buffer(paquete->buffer, &id, sizeof(int));
	add_to_buffer(paquete->buffer, &program_counter, sizeof(unsigned int));
	enviar_paquete(paquete, socket_memoria);
}

uint32_t traducir_dir_logica_a_fisica(uint32_t dir_logica, t_contexto* contexto)
{
	int nro_pagina = floor(dir_logica / tam_pagina);
	uint32_t desplazamiento = dir_logica - nro_pagina * tam_pagina;
	int nro_marco_solicitado = 0;
	uint32_t dir_fisica = 0;
	solicitar_marco_a_memoria(nro_pagina, contexto->id);
	
	op_code codigo = recibir_codigo_operacion(socket_memoria);
	if (codigo == PAGE_FAULT) return -1;
	else if (codigo == OK)
	{
		recv(socket_memoria, &nro_marco_solicitado, sizeof(int), MSG_WAITALL);
		log_info(logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d.", contexto->id, nro_pagina, nro_marco_solicitado);
		dir_fisica = nro_marco_solicitado * tam_pagina + desplazamiento;
		return dir_fisica;
	} 
	else { log_error(logger, "Código inválido recibido luego de solicitar marco."); exit(EXIT_FAILURE); }
}

void solicitar_marco_a_memoria(int nro_pagina, int pid)
{
    t_paquete* paquete = crear_paquete_codigo_operacion(SOLICITAR_VALOR);
	add_to_buffer(paquete->buffer, &pid, sizeof(int));
	add_to_buffer(paquete->buffer, &nro_pagina, sizeof(int));
	enviar_paquete(paquete, socket_memoria);
}

t_register string_a_registro(const char* string) 
{
    if (strcmp(string, "AX") == 0) {
        return AX;
    } else if (strcmp(string, "BX") == 0) {
        return BX;
    } else if (strcmp(string, "CX") == 0) {
        return CX;
    } else if (strcmp(string, "DX") == 0) {
        return DX;
	} else { log_error(logger, "Se intentó convertir string a registro un parámetro que no es registro."); exit(EXIT_FAILURE); }
}

t_instruction* deserializar_instruccion(void* puntero, int pid)
{ 
	char* log = NULL;

	// | operacion | nro_params | param1 | paramN | 
	t_instruction* instruccion =  malloc(sizeof(t_instruction));
	instruccion->operation = 0;
	instruccion->parameters = list_create();
	
	memcpy(&instruccion->operation, puntero, sizeof(int));
	puntero +=  sizeof(int);
	
	log = string_from_format("PID: %d - Ejecutando: %s - ", pid, instruction_strings[instruccion->operation]);

	int nro_params;
	memcpy(&nro_params, puntero, sizeof(int));
	puntero += sizeof(int);

	for(int i = 0; i < nro_params; i++)
	{
		size_t largo_parametro = 0;

		memcpy(&largo_parametro, puntero, sizeof(size_t));
		puntero += sizeof(size_t);

		char* parametro = malloc(largo_parametro);
		memcpy(parametro, puntero, largo_parametro);
		puntero += largo_parametro;
		
		if(i == nro_params) string_append_with_format(&log, "%s.", parametro);
		string_append_with_format(&log, "%s ", parametro);
		
		list_add(instruccion->parameters, parametro);
	}

	log_info(logger, "%s", log);
		     
    return instruccion;
}

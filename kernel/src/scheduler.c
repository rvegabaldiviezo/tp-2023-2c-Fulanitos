#include "scheduler.h"
#define IP_CONFIG_PATH "/home/utnso/tp-2023-2c-Fulanitos/config/ip.config"

pthread_mutex_t logmu = PTHREAD_MUTEX_INITIALIZER;
int pid_global = 0;
int operaciones_memoria_fs = 0;
int base_segmento_respuesta = -1;
sem_t mutex_pid_comparador;
sem_t mutex_deteccion_deadlock;
sem_t* vector_recursos;
int pid_comparador = 0;
int* recursos_disponibles;
bool deadlock = false;
pthread_mutex_t mutex_deadlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
sem_t esperar_manejo;

// ----------------
// INICIALIZACIONES

	void initialize_scheduler(t_config* config_kernel, t_log* logger, int cpu_int, int cpu, int memoria, int fs)
	{
		config = config_kernel;
		logger = logger;
		socket_cpu_plani = cpu;
		socket_cpu_plani_int = cpu_int;
		socket_memoria_plani = memoria;
		socket_fs_plani = fs;

		// Variables config
			char* algoritmo_string = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
			multiprogramacion = config_get_int_value(config,"GRADO_MULTIPROGRAMACION_INI");
			if(strcmp(algoritmo_string, "FIFO") == 0) algoritmo_plani = FIFO;
			else if(strcmp(algoritmo_string, "RR") == 0) 
			{ 
				algoritmo_plani = RR; 
				tiempo_quantum = config_get_int_value(config, "QUANTUM");
			}
			else algoritmo_plani = PRIORIDADES;
			recursos = config_get_array_value(config, "RECURSOS");
			instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");
		// ----

		inicio_semaforos();

		// Inicializar colas de planificación y lista auxiliar
			plani_new = list_create();
			plani_ready = list_create();
			plani_exec = list_create();
			plani_block = list_create();
			plani_exit = list_create();
			total_pcbs = list_create();
		// ----

		// Inicializar recursos
			int n = total_recursos();
			recursos_disponibles = malloc(n * sizeof(int));
			colas_recursos = malloc(sizeof(t_list*) * n); // en realidad es una lista para poder desencolar al proceso cuando se lo finaliza por consola (puede no estar en primer lugar de una cola)
			for(int i = 0; i < n; i++) colas_recursos[i] = list_create();
			vector_recursos = (sem_t*)malloc(n * sizeof(sem_t));
			for (int i = 0; i < n; ++i)
			{
				int instancias_disponibles = atoi(instancias_recursos[i]);
				if (sem_init(&vector_recursos[i], 0, instancias_disponibles) == -1) {
					log_error(logger, "No se pudo inicializar semáforo de recurso.");
					exit(EXIT_FAILURE);
				}
			}
		// ----

		tabla_archivos_global = list_create();

		log_info(logger, "PAUSA DE PLANIFICACION");
		pthread_t hilo_plani_largo_plazo;
		pthread_create(&hilo_plani_largo_plazo, NULL, (void *)inicio_plani_largo_plazo, NULL);
		pthread_detach(hilo_plani_largo_plazo);

		pthread_t hilo_plani_corto_plazo;
		pthread_create(&hilo_plani_corto_plazo, NULL, (void *)inicio_plani_corto_plazo, NULL);
		pthread_detach(hilo_plani_corto_plazo);
	}

	void inicio_semaforos()
	{
		sem_init(&mutex_pid_comparador, 0, 1);
		sem_init(&mutex_deteccion_deadlock, 0, 1);
		sem_init(&mutex_multiprog_exit, 0, 1);
		sem_init(&mutex_multiprog, 0, 1);
		sem_init(&multiprog_disponible, 0, multiprogramacion);

		sem_init(&pcbs_en_new, 0, 0);
		sem_init(&pcbs_en_ready, 0, 0);
		sem_init(&sem_total_pcbs, 0, 1);
		sem_init(&puede_ejecutar, 0, 0);
		sem_init(&esperar_manejo, 0, 1);

		sem_init(&sem_new, 0, 1);
		sem_init(&sem_ready, 0, 1);
		sem_init(&sem_exec, 0, 1);
		sem_init(&sem_block, 0, 1);
		sem_init(&sem_exit, 0, 1);

		sem_init(&sem_block_recursos, 0, 1);
		sem_init(&sem_recursos, 0, 1);
		sem_init(&sem_inst_recursos, 0, 1);
		
		sem_init(&sem_detener_planificacion, 0, 1);
		sem_init(&sem_detener_block_ready, 0, 1);
		sem_init(&sem_detener_new_ready, 0, 1);
		sem_init(&sem_detener_execute, 0, 1);
		sem_init(&sem_detener_block, 0, 1);
		
		sem_init(&mutex_socket_fs, 0, 1);
		sem_init(&debug, 0, 1);
		sem_init(&aguardar_deadlock, 0, 0);
	}

// ----------------

void* inicio_plani_largo_plazo()
{
	while(1)
	{
		sem_wait(&pcbs_en_new);
		//sem_wait(&mutex_multiprog);
		sem_wait(&multiprog_disponible);
		//sem_post(&mutex_multiprog);
		sem_wait(&sem_detener_new_ready);	
		sem_post(&sem_detener_new_ready);
		t_pcb* pcb = sacar_siguiente_de_new();
		agregar_a_ready(pcb);
	}
	return NULL;
}

void* inicio_plani_corto_plazo()
{
	t_pcb* pcb = NULL;
	bool replanificar = true;
	uint32_t tamanio = 0;
	
	while(1)
	{			
		if(replanificar)
		{
			log_debug(logger, "Replanificando");
			sem_wait(&pcbs_en_ready);
			sem_wait(&puede_ejecutar);

			sem_wait(&sem_exec);
			int exec = list_size(plani_exec);
			sem_post(&sem_exec);
			if(exec > 0) sacar_de_exec();

			sem_wait(&sem_ready);
			if(list_is_empty(plani_ready)) 
			{				
				sem_post(&sem_ready);	
				continue;	
			}

			log_debug(logger, "Sacando proceso de ready");
			pcb = sacar_de_ready();	
			sem_post(&sem_ready);

			log_debug(logger, "Mandando a ejecutar");
			agregar_a_exec(pcb);
		}
		else
		{
			pcb = pcb_de_exec();
			replanificar = true;
		}
		armar_y_enviar_contexto(pcb, socket_cpu_plani);

		if(algoritmo_plani == RR)
		{
			pthread_t hilo_quantum;
			pthread_create(&hilo_quantum, NULL, (void *)iniciar_quantum, (void*)pcb);
			pthread_detach(hilo_quantum);
		}			
			
		recibir_contexto_y_actualizar_pcb(pcb);
		//log_debug(logger, "Contexto recibido pcb actualizado");
		op_code codigo = recibir_codigo_operacion(socket_cpu_plani);
		//log_debug(logger, "Codigo %d", codigo);

		sem_wait(&sem_detener_execute); 
		sem_post(&sem_detener_execute); 

		switch(codigo)
		{
			case PROCESS_FINISHED:
			{				
				cambiar_estado_pcb(pcb, FIN);
				pcb->manejando_contexto = false;
				finalizar_proceso(pcb, "SUCCESS"); 
				break;
			}
			case PAGE_FAULT: 
			{
				int pagina = 0;
				recv(socket_cpu_plani, &pagina, sizeof(int), MSG_WAITALL);
				log_info(logger, "Page Fault PID: %d - Pagina: %d", pcb->id, pagina);
				int params[] = {pcb->id, pagina};

				agregar_a_block(pcb);	
				pcb->manejando_contexto = false;
				pthread_t hilo_pf;
				pthread_create(&hilo_pf, NULL, (void *)handle_pf, (void*)params);
				pthread_detach(hilo_pf);
				
				replanificar = true;
				break;
			}
			case SOLICITAR_WAIT: 
			{
				size_t tamanio_recurso = 0;
				recv(socket_cpu_plani, &tamanio_recurso, sizeof(size_t), MSG_WAITALL);
				char* recurso_wait = malloc(tamanio_recurso);
				recv(socket_cpu_plani, recurso_wait, tamanio_recurso, MSG_WAITALL);
					
				int i = posicion_del_recurso(recurso_wait);
				if(i != -1)
				{
					int instancias_actuales = 0;
					int instancias_nuevas = 0;
					sem_wait(&sem_inst_recursos);
					sem_getvalue(&vector_recursos[i], &instancias_actuales); // por si pide recurso en 0
					sem_post(&sem_inst_recursos);
					if(instancias_actuales > 0)
					{
						sem_wait(&sem_inst_recursos);
						sem_wait(&vector_recursos[i]);
						sem_getvalue(&vector_recursos[i], &instancias_nuevas);
						sem_post(&sem_inst_recursos);
						pcb->recursos_asignados[i] += 1;

						log_info(logger, "PID: %d - Wait: %s - Instancias disponibles: %d", pcb->id, recurso_wait, instancias_nuevas);
						
						send_op_code(socket_cpu_plani, OK);
						pcb->manejando_contexto = false;
						replanificar = false;
					}
					else
					{
						pcb->recurso_requerido = recursos[i];
						t_pcb* pcb_sacado =	sacar_de_exec();
						agregar_a_block_recurso(pcb_sacado, i);

						log_info(logger, "PID: %d - Bloqueado por: %s", pcb_sacado->id, recurso_wait);
						
						send_op_code(socket_cpu_plani, BLOQUEADO);
						pcb->manejando_contexto = false;
						//sem_wait(&sem_ready); TODO: tendrian que estar pero se bloquea con el wait de agregar a ready (cuando se desbloquea algun proceso por solucion de deadlock)
						ciclo_deteccion_deadlock(); // espera a solucionar deadlock para seguir planificacion
						//sem_post(&sem_ready);
					}
				}
				else 
				{
					send_op_code(socket_cpu_plani, FIN_PROCESO);
					cambiar_estado_pcb(pcb, FIN);
					pcb->manejando_contexto = false;
					finalizar_proceso(pcb, "INVALID RESOURCE.");
				}
				break;
			}
			case SOLICITAR_SIGNAL: 
			{
				int tamanio_recurso = 0;
				recv(socket_cpu_plani, &tamanio_recurso, sizeof(size_t), MSG_WAITALL);
				char* recurso_signal = malloc(tamanio_recurso);
				recv(socket_cpu_plani, recurso_signal, tamanio_recurso, MSG_WAITALL);

				int i = posicion_del_recurso(recurso_signal);
				if(i != -1)
				{
					if(pcb->recursos_asignados[i] > 0)
					{						
						int resultado_signal = 0;

						sem_wait(&sem_inst_recursos);
						sem_post(&vector_recursos[i]);						
						sem_getvalue(&vector_recursos[i], &resultado_signal);
						sem_post(&sem_inst_recursos);

						pcb->recursos_asignados[i] -= 1;

						log_info(logger, "PID: %d - Signal: %s - Instancias disponibles: %d", pcb->id, recurso_signal, resultado_signal);
						//sem_wait(&sem_block_recursos);
						if(!list_is_empty(colas_recursos[i])) // TODO: puede chocarse con sacar de block recurso cuando finalizo un proceso por consola
						{
							//sem_post(&sem_block_recursos);
							sacar_de_block_recurso(i);
						}
						
						send_op_code(socket_cpu_plani, OK);
						pcb->manejando_contexto = false;
						replanificar = false;
					}
					else 
					{
						send_op_code(socket_cpu_plani, FIN_PROCESO);
						cambiar_estado_pcb(pcb, FIN);
						pcb->manejando_contexto = false;
						finalizar_proceso(pcb, "SIGNAL de recurso no asignado.");
					}
				}
				else 
				{
					send_op_code(socket_cpu_plani, FIN_PROCESO);
					cambiar_estado_pcb(pcb, FIN);
					finalizar_proceso(pcb, "INVALID RESOURCE.");
				}
				break; 
			}			
			case OPEN:
			{
				size_t largo_archivo = 0;
				char modo;

				recv(socket_cpu_plani, &largo_archivo, sizeof(size_t), MSG_WAITALL);
				char* archivo = malloc(largo_archivo); 
				recv(socket_cpu_plani, archivo, largo_archivo, MSG_WAITALL);
				recv(socket_cpu_plani, &modo, sizeof(char), MSG_WAITALL);

				log_info(logger, "PID: %d - Abrir Archivo: %s", pcb->id, archivo);

				t_paquete* paquete = crear_paquete_codigo_operacion(OPEN);
				add_to_buffer(paquete->buffer, &largo_archivo, sizeof(size_t));
				add_to_buffer(paquete->buffer, archivo, largo_archivo);

				int socket_fileSystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
				log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_fileSystem);
				enviar_paquete(paquete, socket_fileSystem);

				entrada_archivo_global* entrada_global = NULL;
				codigo = recibir_codigo_operacion(socket_fileSystem);
				close(socket_fileSystem);	
				log_info(logger, "Conexión con File System %d cerrada.", socket_fileSystem);       

				if (codigo == NO_SUCH_FILE)
				{
					t_paquete* p = crear_paquete_codigo_operacion(CREATE);
					add_to_buffer(p->buffer, &largo_archivo, sizeof(size_t));
					add_to_buffer(p->buffer, archivo, largo_archivo);						

					socket_fileSystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
					log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_fileSystem);

					enviar_paquete(p, socket_fileSystem);						

					validar_codigo_recibido(socket_fileSystem, OK, logger);
					close(socket_fileSystem);
					log_info(logger, "Conexión con File System %d cerrada.", socket_fileSystem);       
				}
				
				entrada_global = buscar_archivo_tabla_global(archivo);
				if(entrada_global == NULL) 
				{
					log_debug(logger, "Creando entrada");
					entrada_global = crear_entrada_global(archivo);
					list_add(tabla_archivos_global, entrada_global);
				}
				entrada_archivo* entrada_pcb = crear_entrada_tabla_pcb(pcb, entrada_global);
					
				if (modo == 'R') 
				{
					entrada_pcb->modo = 'R';
					if (!crear_lock_lectura(entrada_global, pcb)) 
					{
						send_op_code(socket_cpu_plani, OK);
						replanificar = false;
					}
					else 
					{
						send_op_code(socket_cpu_plani, BLOQUEADO);
						log_info(logger, "PID: %d - Bloqueado por: %s", pcb->id, archivo);
						agregar_a_block(pcb);
						pcb->block_archivo = true;
						pcb->manejando_contexto = false;
					}
				}
				else if (modo == 'W') 
				{
					entrada_pcb->modo = 'W';
					if (!crear_lock_escritura(entrada_global, pcb))
					{
						send_op_code(socket_cpu_plani, OK);
						replanificar = false;
					}
					else 
					{
						send_op_code(socket_cpu_plani, BLOQUEADO);
						log_info(logger, "PID: %d - Bloqueado por: %s", pcb->id, archivo);
						agregar_a_block(pcb);
						pcb->block_archivo = true;
						pcb->manejando_contexto = false;
					}
				}
				else 
				{
					log_error(logger, "Modo inválido. No se ejecuta instrucción."); 
					exit(EXIT_FAILURE);
				}
				break;
			}
			case READ: 
			{
				size_t largo_nombre = 0;
				recv(socket_cpu_plani, &largo_nombre, sizeof(size_t), MSG_WAITALL);
				char* archivo = malloc(largo_nombre);
				recv(socket_cpu_plani, archivo, largo_nombre, MSG_WAITALL);
				uint32_t dir_fisica = 0;
				recv(socket_cpu_plani, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
				recv(socket_cpu_plani, &tamanio, sizeof(uint32_t), MSG_WAITALL);	
				int pagina = 0;
				recv(socket_cpu_plani, &pagina, sizeof(int), MSG_WAITALL);				

				entrada_archivo* a = buscar_archivo_tabla_pcb(pcb->tabla_archivos_abiertos, archivo);

				log_info(logger, "PID: %d - Leer Archivo: %s - Puntero: %d - Dirección Memoria: %d - Tamanio: %d",  pcb->id, archivo, a->puntero_en_archivo, dir_fisica, tamanio);
				
				t_paquete* paquete = crear_paquete_codigo_operacion(READ);
				add_to_buffer(paquete->buffer, &pcb->id, sizeof(int));
				add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
				add_to_buffer(paquete->buffer, archivo, largo_nombre);
				add_to_buffer(paquete->buffer, &a->puntero_en_archivo, sizeof(uint32_t));
				add_to_buffer(paquete->buffer, &dir_fisica, sizeof(uint32_t));
				add_to_buffer(paquete->buffer, &pagina, sizeof(int));

				int socket_fileSystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
				log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_fileSystem);
				enviar_paquete(paquete, socket_fileSystem); 

				agregar_a_block(pcb);
				pcb->manejando_contexto = false;
				pthread_t hilo_lectura;
				arg_fs* args = (arg_fs*)malloc(sizeof(arg_fs));
				args->pcb = pcb;
				args->socket = socket_fileSystem;
				pthread_create(&hilo_lectura, NULL, esperar_fs, (void*) args);
				pthread_detach(hilo_lectura);
				
				free(archivo);
				break;
			}
			case WRITE:
			{
				size_t largo_nombre = 0;
				recv(socket_cpu_plani, &largo_nombre, sizeof(size_t), MSG_WAITALL);
				char* archivo = malloc(largo_nombre);
				recv(socket_cpu_plani, archivo, largo_nombre, MSG_WAITALL);
				uint32_t dir_fisica = 0;
				recv(socket_cpu_plani, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
				recv(socket_cpu_plani, &tamanio, sizeof(uint32_t), MSG_WAITALL);
				int pagina = 0;
				recv(socket_cpu_plani, &pagina, sizeof(int), MSG_WAITALL);				

				entrada_archivo* a = buscar_archivo_tabla_pcb(pcb->tabla_archivos_abiertos, archivo);
				log_info(logger, "PID: %d - Escribir Archivo: %s - Puntero: %d - Dirección Memoria: %d - Tamanio: %d",  pcb->id, archivo, a->puntero_en_archivo, dir_fisica, tamanio);
				char modo = a->modo;
				if (modo == 'W')
				{
					send_op_code(socket_cpu_plani, OK);

					t_paquete* paquete = crear_paquete_codigo_operacion(WRITE);
					add_to_buffer(paquete->buffer, &pcb->id, sizeof(int));
					add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
					add_to_buffer(paquete->buffer, archivo, largo_nombre);
					add_to_buffer(paquete->buffer, &a->puntero_en_archivo, sizeof(uint32_t));
					add_to_buffer(paquete->buffer, &dir_fisica, sizeof(uint32_t));
					add_to_buffer(paquete->buffer, &pagina, sizeof(int));
					
					int socket_fileSystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
					log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_fileSystem);
					enviar_paquete(paquete, socket_fileSystem);

					agregar_a_block(pcb);
					pcb->manejando_contexto = false;
					pthread_t hilo_escritura;
					arg_fs* args = (arg_fs*)malloc(sizeof(arg_fs));
					args->pcb = pcb;
					args->socket = socket_fileSystem;
					pthread_create(&hilo_escritura, NULL, esperar_fs, (void*) args);
					pthread_detach(hilo_escritura);
				}
				else if (modo == 'R') 
				{
					send_op_code(socket_cpu_plani, FIN_PROCESO);
					cambiar_estado_pcb(pcb, FIN);
					pcb->manejando_contexto = false;
					finalizar_proceso(pcb, "INVALID_WRITE");
				}

				free(archivo);
				break;
			}
			case TRUNCATE:
			{
				size_t largo_nombre = 0;
				recv(socket_cpu_plani, &largo_nombre, sizeof(size_t), MSG_WAITALL);
				char* archivo = malloc(largo_nombre);
				recv(socket_cpu_plani, archivo, largo_nombre, MSG_WAITALL);
				uint32_t tamanio_truncado = 0;
				recv(socket_cpu_plani, &tamanio_truncado, sizeof(uint32_t), MSG_WAITALL);

				log_info(logger,  "PID: %d - Archivo: %s - Tamaño: %d", pcb->id, archivo, tamanio_truncado);

				t_paquete* paquete = crear_paquete_codigo_operacion(TRUNCATE);
				add_to_buffer(paquete->buffer, &largo_nombre, sizeof(size_t));
				add_to_buffer(paquete->buffer, archivo, largo_nombre);
				add_to_buffer(paquete->buffer, &tamanio_truncado, sizeof(uint32_t));
				
				int socket_fileSystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
				log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_fileSystem);
				enviar_paquete(paquete, socket_fileSystem);
				free(archivo);

				agregar_a_block(pcb);
				pcb->manejando_contexto = false;
				pthread_t hilo_truncado;
				arg_fs* args = (arg_fs*)malloc(sizeof(arg_fs));
				args->pcb = pcb;
				args->socket = socket_fileSystem;
				pthread_create(&hilo_truncado, NULL, esperar_fs, (void*) args);
				pthread_detach(hilo_truncado);

				break;
			}
			case CLOSE:
			{
				size_t largo_nombre = 0;
				recv(socket_cpu_plani, &largo_nombre, sizeof(size_t), MSG_WAITALL);
				char* archivo = malloc(largo_nombre);
				recv(socket_cpu_plani, archivo, largo_nombre, MSG_WAITALL);

				log_info(logger, "PID: %d - Cerrar Archivo: %s", pcb->id, archivo);

				entrada_archivo* a = buscar_archivo_tabla_pcb(pcb->tabla_archivos_abiertos, archivo);
				entrada_archivo_global* e = a->puntero_a_tabla_global;
				char modo = a->modo;
				if (modo == 'W') eliminar_lock_escritura(a->puntero_a_tabla_global);
				else if (modo == 'R') eliminar_lock_lectura(a->puntero_a_tabla_global);
				
				borrar_entrada_tabla_pcb(pcb, archivo);
				log_debug(logger, "A cerrar");
				//pthread_mutex_lock(&e->lock);
				if (e->lectores == 0 && e->escritores == 0) 
				{
					log_debug(logger, "Borrando entrada");
					borrar_entrada_global(e);
				}
				//pthread_mutex_unlock(&e->lock);
				log_debug(logger, "Cerrado");
				pcb->manejando_contexto = false;
				replanificar = false;
				free(archivo);

				break;
			}
			case SEEK:
			{
				size_t largo_nombre = 0;
				recv(socket_cpu_plani, &largo_nombre, sizeof(size_t), MSG_WAITALL);
				char* archivo = malloc(largo_nombre);
				recv(socket_cpu_plani, archivo, largo_nombre, MSG_WAITALL);
				uint32_t puntero = 0;
				recv(socket_cpu_plani, &puntero, sizeof(uint32_t), MSG_WAITALL);

				log_info(logger, "PID: %d - Actualizar puntero Archivo: %s - Puntero: %d", pcb->id, archivo, puntero);

				entrada_archivo* a = buscar_archivo_tabla_pcb(pcb->tabla_archivos_abiertos, archivo);
				a->puntero_en_archivo = puntero;

				pcb->manejando_contexto = false;
				replanificar = false;
				free(archivo);
				break;
			}
			case SLEEP_BLOQUEANTE:
			{
				int tiempo = 0;
				recv(socket_cpu_plani, &tiempo, sizeof(int), MSG_WAITALL);

				log_info(logger, "PID: %d - Bloqueado por: SLEEP", pcb->id); 

				pcb = sacar_de_exec();
				agregar_a_block(pcb);
				pcb->manejando_contexto = false;
				crear_hilo_bloqueo(pcb, tiempo);

				break;
			}
			case DESALOJO:
			{
				//sacar_de_exec();
				pcb->interrupcion = SIN_INT;
				if(pcb->estado != FIN) agregar_a_ready(pcb);
				pcb->manejando_contexto = false;

				break;
			}
			default:
			{
				log_error(logger, "Hubo un error en la comunicación con el CPU");
				log_destroy(logger);
				exit(-1);

				break;
			}
		}
    }
    return NULL;   
}

// ---
// NEW

	void* crear_proceso(char* path, int size, int prioridad)
	{
		t_pcb* nuevo_pcb = malloc(sizeof(t_pcb));
		nuevo_pcb->id = pid_global; pid_global++;
		nuevo_pcb->size = size;
		nuevo_pcb->program_counter = 0;
		nuevo_pcb->registros[AX] = 0;
		nuevo_pcb->registros[BX] = 0;
		nuevo_pcb->registros[CX] = 0;
		nuevo_pcb->registros[DX] = 0;
		nuevo_pcb->prioridad = prioridad;
		nuevo_pcb->tabla_archivos_abiertos = list_create();
		nuevo_pcb->estado = NEW;
		int n = total_recursos();
		nuevo_pcb->recurso_requerido = NULL;
		nuevo_pcb->recursos_asignados = malloc(n * sizeof(int));
		nuevo_pcb->recursos_pedidos = malloc(n * sizeof(int));
		for (int i = 0; i < n; ++i) 
		{
			nuevo_pcb->recursos_asignados[i] = 0; 
			nuevo_pcb->recursos_pedidos[i] = 0;
		}
		nuevo_pcb->path = path;
		nuevo_pcb->block_recurso = false;
		nuevo_pcb->block_pf = false;
		nuevo_pcb->fin_por_consola = false;
		
		crear_proceso_memoria(nuevo_pcb->path, nuevo_pcb->size);

		sem_wait(&sem_total_pcbs);
		list_add(total_pcbs, nuevo_pcb);
		sem_post(&sem_total_pcbs);
		agregar_a_new(nuevo_pcb);

		log_debug(logger, "Proceso %d agregado a NEW", nuevo_pcb->id);
		
		return NULL;
	}

	void agregar_a_new(t_pcb* nuevo_pcb)
	{
		sem_wait(&sem_new);
		list_add(plani_new, nuevo_pcb);
		sem_post(&sem_new);
		sem_post(&pcbs_en_new);
	}

	t_pcb* sacar_siguiente_de_new()
	{
		sem_wait(&sem_new);
		t_pcb* pcb = (t_pcb*) list_remove(plani_new, 0);
		log_debug(logger, "PID %d sacado de NEW", pcb->id);
		sem_post(&sem_new);
		return pcb;
	}

	void sacar_de_new_particular(t_pcb* pcb)
	{
		sem_wait(&sem_new);
		list_remove_element(plani_new, pcb);
		sem_post(&sem_new);
	}

// ---

// -----
// READY

	void agregar_a_ready(t_pcb* pcb)
	{		
		if(algoritmo_plani == PRIORIDADES)
		{
			sem_wait(&sem_exec);
			int exec = list_size(plani_exec);
			sem_post(&sem_exec);
			if(exec > 0) 
			{
				t_pcb* pcb_exec = list_get(plani_exec, 0);
				if(pcb_exec->prioridad > pcb->prioridad)
				enviar_interrupcion(pcb_exec->id, DESALOJO_PRIORIDADES);
			}
		}
		sem_wait(&sem_ready);
		list_add(plani_ready, pcb);
		sem_post(&sem_ready);

		sem_post(&pcbs_en_ready);
		cambiar_estado_pcb(pcb, READY);
		mostrar_log_agregado_ready();		
		sem_post(&puede_ejecutar);
	}

	t_pcb* sacar_de_ready()
	{
		t_pcb* pcb;
		switch(algoritmo_plani){
		case FIFO:			
			pcb = list_remove(plani_ready, 0);
			return pcb;
		case RR: 
			pcb = list_remove(plani_ready, 0);
			return pcb;
		case PRIORIDADES: 
			int i = list_size(plani_ready);
			if(i == 1)
			{
				pcb = list_remove(plani_ready, 0);
				return pcb;
			} 
			pcb = (t_pcb*)list_get_minimum(plani_ready, comparar_prioridad);
			log_debug(logger, "Encontre mayor prioridad");
			list_remove_element(plani_ready, pcb);
			log_debug(logger, "Saco de ready");
			return pcb;    
		default:
			return NULL;
		}
	}

	void sacar_de_ready_particular(t_pcb* pcb)
	{
		sem_wait(&sem_ready);
		list_remove_element(plani_ready, pcb);
		sem_post(&sem_ready);
	}

// -----

// -------
// EXECUTE

	void agregar_a_exec(t_pcb* pcb)
	{
		sem_wait(&sem_exec);
		list_add(plani_exec, pcb);
		sem_post(&sem_exec);
		cambiar_estado_pcb(pcb, EXEC);
	}

	t_pcb* sacar_de_exec()
	{
		sem_wait(&sem_exec);
		t_pcb* pcb = list_remove(plani_exec, 0);
		sem_post(&sem_exec);
		return pcb;
	}

	t_pcb* pcb_de_exec()
	{
		sem_wait(&sem_exec);
		t_pcb* pcb = list_get(plani_exec, 0);
		sem_post(&sem_exec);
		return pcb;
	}

// -------

// -----
// BLOCK

	void agregar_a_block(t_pcb* pcb)
	{
		sem_wait(&sem_block);
		list_add(plani_block, pcb);
		sem_post(&sem_block);
		cambiar_estado_pcb(pcb, BLOCK);
	}
	t_pcb* sacar_de_block(t_pcb* pcb)
	{
		sem_wait(&sem_detener_block); 
		sem_post(&sem_detener_block); 
		sem_wait(&sem_block);
		list_remove_element(plani_block, pcb);
		sem_post(&sem_block);
		return pcb;
	}
	t_pcb* sacar_de_block_por_id(int pid)
	{
		t_pcb* pcb;
		int encontro = 0;
		sem_wait(&sem_block);
		for(int i = 0; i < list_size(plani_block) && encontro == 0; i++){
			if(((t_pcb*)list_get(plani_block, i))->id == pid){
				pcb = list_remove(plani_block, i);
				encontro = 1;
			}
		}
		sem_post(&sem_block);
		return pcb;
	}
	void crear_hilo_bloqueo(t_pcb* pcb, int tiempo)
	{
		arg_sleep* args = (arg_sleep*)malloc(sizeof(arg_sleep));
		args->pcb = pcb;
		args->tiempo = tiempo;

		pthread_t hilo_bloqueo;
		pthread_create(&hilo_bloqueo, NULL, (void *)bloquear_proceso_sleep, (void*)args);
		pthread_detach(hilo_bloqueo);
	}
	void* bloquear_proceso_sleep(void* args)
	{
		arg_sleep* arg = (arg_sleep*)args;
		t_pcb* pcb = arg->pcb;
		int tiempo = arg->tiempo;

		sleep(tiempo);
		pcb = sacar_de_block(pcb);
		agregar_a_ready(pcb);

		free(arg);
		return NULL;
	}

// -----

// ----
// EXIT

	void agregar_a_exit(void* arg)
	{
		arg_fin* args = (arg_fin*)arg;
		t_pcb* pcb = args->pcb;
		char* motivo = args->motivo;
		t_estado estado_aux = pcb->estado;	

		if(pcb->fin_por_consola)
		{
			while(pcb->manejando_contexto) sleep(1); // TODO: chequear espera activa
			switch(pcb->estado) 
			{
				case EXEC: 
				{ 
					enviar_interrupcion(pcb->id, FIN_PROCESO); 
					recibir_contexto(socket_cpu_plani);
					//sacar_de_exec(); 
					break; 
				} 
				case READY: { sacar_de_ready_particular(pcb); break; } // si resolvio pf lo manda a ready. pcp puede pasar el sem pcbs en ready y tratar de sacar el pcb que estoy eliminando aca
				case BLOCK: { sacar_de_block_por_id(pcb->id); break; } 
				case NEW: { sacar_de_new_particular(pcb); break; }
				default: { log_error(logger, "Estado inválido. Se finalizará el proceso de todas formas."); break; }
			}
			cambiar_estado_pcb(pcb, FIN);
		}
		
		log_debug(logger, "A liberar proceso");
		if(estado_aux == EXEC && pcb->fin_por_consola)
		{
			int multi_disponible = 0;
			sem_wait(&mutex_multiprog);
			sem_getvalue(&multiprog_disponible, &multi_disponible);
			if(multiprogramacion > multi_disponible + 1)
				sem_post(&multiprog_disponible);
			sem_post(&mutex_multiprog);
		}
		else 
		{
			//sem_wait(&mutex_multiprog);
			sem_post(&multiprog_disponible);
			//sem_post(&mutex_multiprog);
		}

		sem_wait(&sem_exit);
		list_add(plani_exit, pcb); 
		sem_post(&sem_exit);
		log_debug(logger, "Agregado a exit");
		
		liberar_recursos_asignados(pcb);
		borrar_tabla_archivos_abiertos_pcb(pcb);
		liberar_memoria(pcb);

		log_info(logger, "Finaliza el proceso %d - Motivo: %s", pcb->id, motivo);

		sem_wait(&sem_total_pcbs);
		list_remove_element(total_pcbs, pcb);
		sem_post(&sem_total_pcbs);

		free(pcb);
		free(args);

		if(pcb->fin_por_consola) 
		{
			sem_wait(&mutex_deteccion_deadlock);
			detectar_deadlock_recursos();
			sem_post(&mutex_deteccion_deadlock);
			
			sem_post(&aguardar_deadlock);
		}
	}

	void enviar_interrupcion(int pid, t_int interrupcion)
	{
		t_paquete* paquete = crear_paquete_codigo_operacion(INTERRUPCION);
		add_to_buffer(paquete->buffer, &interrupcion, sizeof(t_int));
		add_to_buffer(paquete->buffer, &pid, sizeof(int));
		enviar_paquete(paquete, socket_cpu_plani_int);
	}

	void finalizar_proceso(t_pcb* pcb, char* motivo)
	{
		arg_fin* args = (arg_fin*)malloc(sizeof(arg_fin));
		args->pcb = pcb;
		args->motivo = motivo;

		log_info(logger, "Finaliza el proceso: %d - Motivo: %s", pcb->id, args->motivo);

		pthread_t hilo_finalizar_proceso;
		pthread_create(&hilo_finalizar_proceso, NULL, (void*)agregar_a_exit, (void*)args);
		pthread_detach(hilo_finalizar_proceso);
	}

// ----

// --------
// RECURSOS

	int total_recursos()
	{
		int total_de_recursos;
		sem_wait(&sem_recursos);
		total_de_recursos = string_array_size(recursos);
		sem_post(&sem_recursos);
		return total_de_recursos;
	}

	int posicion_del_recurso(char* recurso)
	{
		int total_de_recursos = total_recursos();
		sem_wait(&sem_recursos);
		for(int i = 0; i < total_de_recursos; i++)
		{
			if(strcmp(recurso, recursos[i]) == 0)
			{
				sem_post(&sem_recursos);
				return i;
			}
		}
		sem_post(&sem_recursos);
		return -1;
	}

	void agregar_a_block_recurso(t_pcb* pcb, int i)
	{
		sem_wait(&sem_block_recursos);
		list_add(colas_recursos[i], pcb);
		sem_post(&sem_block_recursos);

		pcb->recursos_pedidos[i] = 1;
		pcb->block_recurso = true;
		cambiar_estado_pcb(pcb, BLOCK);
	}

	void sacar_de_block_recurso(int recurso)
	{		
		t_pcb* pcb = NULL;

		sem_wait(&sem_block_recursos);
		if(!list_is_empty(colas_recursos[recurso]))
		{
			pcb = (t_pcb*)list_remove(colas_recursos[recurso], list_size(colas_recursos[recurso]) - 1);	
			log_info(logger, "PID %d desbloqueado de cola %s.", pcb->id, recursos[recurso]);

			int instancias_nuevas = 0;
			sem_wait(&sem_inst_recursos);
			sem_wait(&vector_recursos[recurso]);
			sem_getvalue(&vector_recursos[recurso], &instancias_nuevas);
			sem_post(&sem_inst_recursos);

			pcb->recursos_asignados[recurso] += 1;
			log_info(logger, "PID: %d - Wait: %s - Instancias disponibles: %d", pcb->id, recursos[recurso], instancias_nuevas);	

			pcb->block_recurso = false;			
			agregar_a_ready(pcb);
		}
		sem_post(&sem_block_recursos);
	}

	void liberar_recursos_asignados(t_pcb* pcb)
	{
		if(pcb->recurso_requerido != NULL)
		{
			int pos = posicion_del_recurso(pcb->recurso_requerido);
			sem_wait(&sem_block_recursos);
			list_remove_element(colas_recursos[pos], pcb);
			sem_post(&sem_block_recursos);
		}

		for(int i = 0; i < total_recursos(); i++) 
		{
			for(int j = 0; j < pcb->recursos_asignados[i]; j++)
			{
				sem_wait(&sem_inst_recursos);
				sem_post(&vector_recursos[i]);
				sem_post(&sem_inst_recursos);

				int* indice_recurso = malloc(sizeof(int));
				*indice_recurso = i;
				pthread_t hilo_liberar_recurso;
				pthread_create(&hilo_liberar_recurso, NULL, aguardar_solucion_deadlock, (void*)indice_recurso);
				pthread_detach(hilo_liberar_recurso);
			}
		}
		free(pcb->recursos_asignados);
	}

	void* aguardar_solucion_deadlock(void* indice)
	{
		int recurso = *(int*)indice;
		sem_wait(&aguardar_deadlock);
		sacar_de_block_recurso(recurso);
		return NULL;
	}

// --------

// -------
// MEMORIA

	void crear_proceso_memoria(char* path, int size)
	{
		size_t largo_path = strlen(path) + 1; 

		t_paquete* paquete = crear_paquete_codigo_operacion(CREAR_PROCESO);
		add_to_buffer(paquete->buffer, &largo_path, sizeof(size_t));
		add_to_buffer(paquete->buffer, path, largo_path);
		add_to_buffer(paquete->buffer, &size, sizeof(int));
		enviar_paquete(paquete, socket_memoria_plani);
		
		free(path);
	}

	void liberar_memoria(t_pcb* pcb)
	{
		t_paquete* paquete = crear_paquete_codigo_operacion(PROCESS_FINISHED);
		add_to_buffer(paquete->buffer, &pcb->id, sizeof(int));
		enviar_paquete(paquete, socket_memoria_plani);
		validar_codigo_recibido(socket_memoria_plani, OK, logger);
	}

	void handle_pf(void* args)
	{
		int* params = (int*)args;
		int pid = params[0];
		int pagina = params[1];

		t_pcb* pcb = buscar_pcb_por_pid(pid);
		pcb->block_pf = true; // TODO: ver si es util. consigna dice que tiene que ser estado independiente, pero creo que en nuestro caso no afecta que este en la misma lista block	
		solicitar_pagina(pagina, pid);		
		if(!pcb->fin_por_consola)		
		{
			sacar_de_block_por_id(pcb->id); 
			agregar_a_ready(pcb);
		}
	}

	void solicitar_pagina(int pagina, int pid)
	{
		t_paquete* paquete = crear_paquete_codigo_operacion(PAGE_FAULT);
		add_to_buffer(paquete->buffer, &pid, sizeof(int));
		add_to_buffer(paquete->buffer, &pagina, sizeof(int));
		enviar_paquete(paquete, socket_memoria_plani);
		validar_codigo_recibido(socket_memoria_plani, OK, logger);
	}

// -------

// --------
// ARCHIVOS

	entrada_archivo_global* crear_entrada_global(char* archivo)
	{
		entrada_archivo_global* e = malloc(sizeof(entrada_archivo_global));
		e->archivo = string_duplicate(archivo);
		inicializar_locks(e);
		return e;
	}

	void borrar_entrada_global(entrada_archivo_global* e)
	{
		free(e->archivo);
		pthread_mutex_destroy(&e->lock);
		pthread_cond_destroy(&e->cola_escritura);
		pthread_cond_destroy(&e->cola_lectura);
		free(e);
	}

	entrada_archivo* crear_entrada_tabla_pcb(t_pcb* pcb, entrada_archivo_global* archivo)
	{
		entrada_archivo* e = malloc(sizeof(entrada_archivo));
		e->archivo = string_duplicate(archivo->archivo);
		e->puntero_a_tabla_global = archivo;
		list_add(pcb->tabla_archivos_abiertos, e);
		return e;
	}

	void borrar_entrada_tabla_pcb(t_pcb* pcb, char* archivo)
	{
		entrada_archivo* e = buscar_archivo_tabla_pcb(pcb->tabla_archivos_abiertos, archivo);
		free(e);
	}

	void borrar_tabla_archivos_abiertos_pcb(t_pcb* pcb)
	{
		int n = list_size(pcb->tabla_archivos_abiertos);
		if(n > 0)
		{
			for(int i = 0; i < n; i++)
				free(list_get(pcb->tabla_archivos_abiertos, i));
			list_destroy(pcb->tabla_archivos_abiertos);
		}
	}

	void* esperar_fs(void* arg) 
	{
		arg_fs* args = (arg_fs*)arg;
		t_pcb* pcb = args->pcb;
		int socket = args->socket;
		validar_codigo_recibido(socket, OK, logger);
		close(socket);
		log_info(logger, "Conexión con File System %d cerrada.", socket);       
		sacar_de_block(pcb);
		agregar_a_ready(pcb);
		free(args);
		return NULL;
	}

	entrada_archivo* buscar_archivo_tabla_pcb(t_list* tabla, char* archivo)
	{
		for (int i = 0; i < list_size(tabla); i++) 
		{
			entrada_archivo* a = list_get(tabla, i);
			if (strcmp(archivo, a->archivo) == 0) return a;
		}
		return NULL;
	}

	entrada_archivo_global* buscar_archivo_tabla_global(char* archivo)
	{
		for (int i = 0; i < list_size(tabla_archivos_global); i++) 
		{
			entrada_archivo_global* a = list_get(tabla_archivos_global, i);
			if (strcmp(archivo, a->archivo) == 0) return a;
		}
		return NULL;
	}

	void inicializar_locks(entrada_archivo_global* archivo) 
	{
		pthread_mutex_init(&archivo->lock, NULL);
		pthread_cond_init(&archivo->cola_lectura, NULL);
		pthread_cond_init(&archivo->cola_escritura, NULL);
		archivo->lectores = 0;
		archivo->escritores = 0;
	}

	bool crear_lock_lectura(entrada_archivo_global* archivo, t_pcb* pcb) 
	{
		log_debug(logger, "Creando lock lectura");
		bool bloqueado = false;
		pthread_mutex_lock(&archivo->lock);
		if (archivo->escritores > 0) 
		{
			bloqueado = true;
			arg_lock* args = (arg_lock*)malloc(sizeof(arg_lock));
			args->archivo = archivo;
			args->pcb = pcb;
			pthread_t hilo_lectura;
			archivo->lectores++;

			log_debug(logger, "Encolando lock lectura");
			pthread_create(&hilo_lectura, NULL, encolar_lock_lectura, (void*)args);
			pthread_detach(hilo_lectura);

			return bloqueado;
		}
		archivo->lectores++;
		pthread_mutex_unlock(&archivo->lock);
		log_debug(logger, "Lock lectura creado");
		return bloqueado;
	}

	bool crear_lock_escritura(entrada_archivo_global* archivo, t_pcb* pcb) 
	{
		log_debug(logger, "Creando lock escritura");
		bool bloqueado = false;
		pthread_mutex_lock(&archivo->lock);
		if (archivo->lectores > 0 || archivo->escritores > 0) 
		{
			bloqueado = true;
			arg_lock* args = (arg_lock*)malloc(sizeof(arg_lock));
			args->archivo = archivo;
			args->pcb = pcb;
			pthread_t hilo_escritura;
			archivo->escritores++;

			log_debug(logger, "Encolando lock escritura");
			pthread_create(&hilo_escritura, NULL, encolar_lock_escritura, (void*)args);
			pthread_detach(hilo_escritura);

			return bloqueado;
		}
		archivo->escritores++;
		pthread_mutex_unlock(&archivo->lock);
		log_debug(logger, "Lock escritura creado");
		return bloqueado;
	}

	void* encolar_lock_lectura(void* arg)
	{
		arg_lock* args = (arg_lock*)arg;
		t_pcb* pcb = args->pcb;
		entrada_archivo_global* archivo = args->archivo;		

		while (archivo->escritores > 0) 
		{	
			pthread_cond_wait(&archivo->cola_lectura, &archivo->lock);
		}
		
		log_debug(logger, "Lock lectura creado");
		list_add(pcb->tabla_archivos_abiertos, archivo);
		sacar_de_block(pcb);
		pcb->block_archivo = false;
		agregar_a_ready(pcb);
		return NULL;
	}

	void* encolar_lock_escritura(void* arg)
	{
		arg_lock* args = (arg_lock*)arg;
		t_pcb* pcb = args->pcb;
		entrada_archivo_global* archivo = args->archivo;		

		while (archivo->lectores > 0 || archivo->escritores > 0)
		{
			pthread_cond_wait(&archivo->cola_escritura, &archivo->lock);
		}
		log_debug(logger, "Lock escritura creado");
		list_add(pcb->tabla_archivos_abiertos, archivo);
		sacar_de_block(pcb);
		pcb->block_archivo = false;
		agregar_a_ready(pcb);
		return NULL;
	}

	void eliminar_lock_lectura(entrada_archivo_global* archivo)
	{
		log_debug(logger, "Eliminando lock lectura");
		pthread_mutex_lock(&archivo->lock);
		log_debug(logger, "Eliminando lock lectura2");
		if (archivo->lectores == 0) 
			pthread_cond_signal(&archivo->cola_escritura);
		archivo->lectores--;
		pthread_mutex_unlock(&archivo->lock);
	}

	void eliminar_lock_escritura(entrada_archivo_global* archivo) 
	{
		log_debug(logger, "Eliminando lock escritura");
 		pthread_mutex_lock(&archivo->lock);
		log_debug(logger, "Eliminando lock escritura2");
		pthread_cond_signal(&archivo->cola_lectura);
		pthread_cond_signal(&archivo->cola_escritura);
		archivo->escritores--;
		pthread_mutex_unlock(&archivo->lock);
	}

// --------

// --------------
// LISTAR ESTADOS

	void listar_general(t_list* lista, char* estado)
	{
		char* ids = string_new();
		t_pcb* pcbAux;
		for(int i = 0; i < list_size(lista); i++){
			pcbAux = list_get(lista, i);
			if(i == 0){
				char* aux = string_new();
				char* numero = string_itoa(pcbAux->id);
				string_append(&aux, numero);
				string_append(&ids, aux);
				free(aux);
				free(numero);
			}else{
				char* aux = string_new();
				char* numero = string_itoa(pcbAux->id);
				string_append(&aux, " , ");
				string_append(&aux, numero);
				string_append(&ids, aux);
				free(aux);
				free(numero);
			}
		}
		log_info(logger, "Estado: %s - Procesos: %s", estado,ids);
		free(ids);
	}
	bool es_new(t_pcb* pcb)
	{
		return pcb->estado == NEW;
	}
	bool es_ready(t_pcb* pcb)
	{
		return pcb->estado == READY;
	}
	bool es_blocked(t_pcb* pcb)
	{
		return pcb->estado == BLOCK;
	}
	bool es_execute(t_pcb* pcb)
	{
		return pcb->estado == EXEC;
	}
	bool es_exit(t_pcb* pcb)
	{
		return pcb->estado == FIN;
	}
	void listar_estados()
	{
		t_list* lista_de_new = list_create();
		t_list* lista_de_ready = list_create();
		t_list* lista_de_blocked = list_create();
		t_list* lista_de_execute = list_create();
		t_list* lista_de_exit = list_create();
		sem_wait(&sem_total_pcbs);
		lista_de_new = list_filter(total_pcbs, (void*) es_new);	
		listar_general(lista_de_new,"NEW");
		lista_de_ready = list_filter(total_pcbs, (void*) es_ready);
		listar_general(lista_de_ready,"READY");
		lista_de_blocked = list_filter(total_pcbs, (void*) es_blocked);
		listar_general(lista_de_blocked,"BLOCK");
		lista_de_execute = list_filter(total_pcbs, (void*) es_execute);
		listar_general(lista_de_execute,"EXECUTE");
		lista_de_exit = list_filter(total_pcbs, (void*) es_exit);
		sem_post(&sem_total_pcbs);
		listar_general(lista_de_exit,"EXIT");
		list_destroy(lista_de_new);
		list_destroy(lista_de_ready);
		list_destroy(lista_de_blocked);
		list_destroy(lista_de_execute);
		list_destroy(lista_de_exit);
	}

	void mostrar_log_agregado_ready()
	{
		// PREPARO STRING DE ALGORITMO
		char* algoritmo = string_new();
		switch(algoritmo_plani){
		case FIFO:
			string_append(&algoritmo, "FIFO");
			break;
		case RR:
			string_append(&algoritmo, "RR");
			break;
		case PRIORIDADES:
			string_append(&algoritmo, "PRIORIDADES");
			break;    
		}
		// PREPARO STRING DE IDS
		char* ids = string_new();
		t_pcb* pcb_aux;
		for(int i = 0; i < list_size(plani_ready); i++){
			pcb_aux = list_get(plani_ready, i);
			if(i == 0){
				char* aux = string_new();
				char* numero = string_itoa(pcb_aux->id);
				string_append(&aux, numero);
				string_append(&ids, aux);
				free(aux);
				free(numero);
			}else{
				char* aux = string_new();
				char* numero = string_itoa(pcb_aux->id);
				string_append(&aux, " - ");
				string_append(&aux, numero);
				string_append(&ids, aux);
				free(aux);
				free(numero);
			}
		}
		log_info(logger, "Cola Ready %s: [%s]", algoritmo, ids);
		free(algoritmo);
		free(ids);
	}

// --------------

// --------
// DEADLOCK

	void ciclo_deteccion_deadlock()
	{
		sem_wait(&mutex_deteccion_deadlock);
		detectar_deadlock_recursos();
		sem_post(&mutex_deteccion_deadlock);

		pthread_mutex_lock(&mutex_deadlock);
		while(deadlock) 
		{
			pthread_cond_wait(&condition, &mutex_deadlock); // se queda aca hasta solucionar deadlock (terminar procesos)
		}
		pthread_mutex_unlock(&mutex_deadlock);
	}

	void detectar_deadlock_recursos()
	{
		log_info(logger, "ANÁLISIS DE DETECCIÓN DE DEADLOCK");
		for (int i = 0; i < total_recursos(); i++) sem_getvalue(&vector_recursos[i], &recursos_disponibles[i]);
		
		sem_wait(&sem_total_pcbs);
		t_list* procesos = list_filter(total_pcbs, bloqueado_por_recurso);
		filtrar_sin_recursos(procesos, recursos_disponibles); // saca los que no tienen asignados y/o peticiones
		
		t_pcb* proceso = NULL;
		while ((proceso = list_find(procesos, (void*)puede_finalizar))) 
		{
			list_remove_element(procesos, proceso);
			liberar_recursos_deadlock(proceso, recursos_disponibles);
		}
		if (list_is_empty(procesos)) 
		{	
			log_info(logger, "No hay deadlock.");
			sem_post(&sem_total_pcbs);
			pthread_mutex_lock(&mutex_deadlock);
			deadlock = false;
			pthread_cond_signal(&condition);
			pthread_mutex_unlock(&mutex_deadlock);
			return;
		}
		else 
		{
			sem_post(&sem_total_pcbs);
			hay_deadlock(procesos);
		}
	}

	void hay_deadlock(t_list* procesos)
	{
		deadlock = true;
		sem_wait(&sem_total_pcbs);
		for(int i = 0; i < list_size(procesos); i++)
		{	
			t_pcb* proceso = list_get(procesos, i);
			char* log = string_from_format("“Deadlock detectado: Proceso %d - Recursos en posesión: ", proceso->id);

			for(int j = 0; j < total_recursos(); j++) 
			{	
				if(j == (total_recursos() - 1))
				{
					if(proceso->recursos_asignados[j] > 0) string_append_with_format(&log, "%s -", recursos[j]);
					string_append_with_format(&log, "Recurso requerido: %s. ", proceso->recurso_requerido);
				}
				else if(proceso->recursos_asignados[j] > 0) string_append_with_format(&log, "%s, ", recursos[j]);
			}

			char* archivos = archivos_afectados(proceso);
			string_append(&log, archivos);
			log_trace(logger, "%s", log);
		}
		sem_post(&sem_total_pcbs);
	}

	char* archivos_afectados(t_pcb* pcb)
	{
		int n = list_size(pcb->tabla_archivos_abiertos);
		if (n > 0)
		{
			char* string = string_from_format("Archivos afectados: ");
			for(int i = 0; i < n; i++)
			{
				entrada_archivo* archivo = (entrada_archivo*)list_get(pcb->tabla_archivos_abiertos, i);
				if(i == n) string_append_with_format(&string, "%s”", archivo->puntero_a_tabla_global->archivo);
				else string_append_with_format(&string, "%s, ", archivo->puntero_a_tabla_global->archivo);
			}
			return string;
		}
		return " ";
	}

	bool puede_finalizar(t_pcb* pcb)
	{
		int n = total_recursos();
		int* recursos_copia = malloc(n * sizeof(int));
		copiar_vector(recursos_disponibles, recursos_copia, n);
		for (int i = 0; i < n; i++) 
		{
			recursos_copia[i] -= pcb->recursos_pedidos[i];
			if (recursos_copia[i] < 0) return false; 
		}
		return true;
	}

	void filtrar_sin_recursos(t_list* procesos, int* recursos_disponibles)
	{
		t_pcb* proceso = NULL;
		for (int i = 0; i < list_size(procesos); i++) 
		{
			proceso = list_get(procesos, i);
			bool a = sumar_vector(proceso->recursos_asignados) == 0;
			bool b = sumar_vector(proceso->recursos_pedidos) == 0;
			if (a || b) 
			{	
				list_remove_element(procesos, proceso);
				if (b) liberar_recursos_deadlock(proceso, recursos_disponibles);
			}	
		}
	}

	void liberar_recursos_deadlock(t_pcb* proceso, int* recursos_disponibles)
	{
		for(int j = 0; j < total_recursos(); j++) 
			recursos_disponibles[j] += proceso->recursos_asignados[j];
	}

	bool bloqueado_por_recurso(void* data) 
	{
		t_pcb* pcb = (t_pcb*)data;
		// int recursos_asignados = 0;
		// for (int i = 0; i < total_recursos(); i++) 
		// 	recursos_asignados += pcb->recursos_asignados[i];
		// bool tiene_recursos = recursos_asignados > 0;
		return pcb->block_recurso == true; // && tiene_recursos;
	}

	void copiar_vector(int source[], int destination[], int size) 
	{
		for (int i = 0; i < size; ++i) destination[i] = source[i];
	}

	int sumar_vector(int* array)
	{
		int suma = 0;
		for (int i = 0; i < total_recursos(); i++) suma += array[i];
		return suma;
	}

// --------

// --------------------------------

	void recibir_contexto_y_actualizar_pcb(t_pcb* pcb)
	{
		validar_codigo_recibido(socket_cpu_plani, CONTEXTO, logger);		
		pcb->manejando_contexto = true;
		t_contexto* contexto = recibir_contexto(socket_cpu_plani);
		actualizar_pcb(pcb, contexto);
	}

	void* iniciar_quantum(void* arg)
	{
		t_pcb* pcb = (t_pcb*) arg;
		int id_aux = pcb->id;
		usleep(1000 * tiempo_quantum);
		// si sigue existiendo el proceso 
		if(id_aux == pcb->id) // TODO: raro
		{
			log_info(logger, "PID: %d - Desalojado por fin de Quantum", pcb->id);
			enviar_interrupcion(pcb->id, FIN_QUANTUM);
		}
		return NULL;
	}

	t_pcb* buscar_pcb_por_pid(int pid)
	{
		sem_wait(&mutex_pid_comparador);
		pid_comparador = pid;
		t_pcb* pcb = list_find(total_pcbs, (void*) igual_pid);
		sem_post(&mutex_pid_comparador);
		return pcb;
	}

	bool igual_pid(t_pcb* pcb) { return pcb->id == pid_comparador; }

	void actualizar_pcb(t_pcb* pcb, t_contexto* contexto)
	{
		pcb->program_counter = contexto->pc;
		pcb->registros[AX] = contexto->registros[AX];
		pcb->registros[BX] = contexto->registros[BX];
		pcb->registros[CX] = contexto->registros[CX];
		pcb->registros[DX] = contexto->registros[DX];
	}

	void crear_nuevo_semaforo_multiprog(int nuevo_grado)
	{
		sem_wait(&mutex_multiprog_exit);
		sem_wait(&mutex_multiprog);
		sem_destroy(&multiprog_disponible);
		sem_init(&multiprog_disponible, 0, nuevo_grado);
		multiprogramacion = nuevo_grado;
		sem_post(&mutex_multiprog_exit);
		sem_post(&mutex_multiprog);
	}

	void detener_planificacion()
	{
		sem_wait(&sem_detener_planificacion);
		sem_wait(&sem_detener_new_ready);
		sem_wait(&sem_detener_block_ready);
		sem_wait(&sem_detener_block);
		sem_wait(&sem_detener_execute);
		sem_post(&sem_detener_planificacion);
	}

	void iniciar_planificacion()
	{
		sem_wait(&sem_detener_planificacion);
		sem_post(&sem_detener_new_ready);
		sem_post(&sem_detener_block_ready);
		sem_post(&sem_detener_block);
		sem_post(&sem_detener_execute);
		sem_post(&sem_detener_planificacion);
	} 

	void* comparar_prioridad(void* arg1, void* arg2) 
	{
		t_pcb* pcb1 = (t_pcb*)arg1;
		t_pcb* pcb2 = (t_pcb*)arg2;
		int prioridad = pcb1->prioridad;
		int prioridad2 = pcb2->prioridad;
		if(prioridad < prioridad2){
			return pcb1;
		}else{
			return pcb2;
		}
	}

	char* estado_a_string(t_estado estado) 
	{
		switch (estado) {
		case NEW:
			return "NEW";
		case READY:
			return "READY";
		case EXEC:
			return "EXEC";
		case BLOCK:
			return "BLOCK";
		case FIN:
			return "EXIT";
		default:
			return "ESTADO DESCONOCIDO";
		}
	}

	void cambiar_estado_pcb(t_pcb* pcb, t_estado estado_nuevo)
	{
		char* estadoAnteriorString = string_new();
		char* estadoNuevoString = string_new();
		t_estado estadoAnterior = pcb->estado;
		pcb->estado = estado_nuevo;
		string_append(&estadoAnteriorString, estado_a_string(estadoAnterior));
		string_append(&estadoNuevoString, estado_a_string(estado_nuevo));
		log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pcb->id, estadoAnteriorString, estadoNuevoString);
		free(estadoAnteriorString);
		free(estadoNuevoString);
	}

// --------------------------------

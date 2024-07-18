#include "kernel.h"
#include "scheduler.h"
extern pthread_mutex_t logmu;
extern sem_t debug;
int main(int argc, char **argv)
{
	initialize_setup(argv[1], "kernel", &logger, &kernel_config);
	initialize_sockets();
	
	initialize_scheduler(kernel_config, logger, socket_cpu_interrupt, socket_cpu_dispatch, socket_memoria, socket_fileSystem);

	add_history("INICIAR_PLANIFICACION"); 
	add_history("DETENER_PLANIFICACION");
	add_history("INICIAR_PROCESO INTEGRAL_A 128 10");
	add_history("INICIAR_PROCESO INTEGRAL_B 128 5");
	add_history("INICIAR_PROCESO INTEGRAL_C 64 1");

	// FILE* script = fopen("/home/utnso/tp-2023-2c-Fulanitos/config/default/comando.txt", "r");
	// char buffer[1024];


	// 		while (fgets(buffer, sizeof(buffer), script) != NULL) 
	// 		{
	// 			size_t len = strlen(buffer);
	// 			if (len > 0 && buffer[len - 1] == '\n') 
	// 				buffer[len - 1] = '\0';
	// 			identificar_linea(buffer);		
	// 		}


	// fclose(script);	

	while(socket_memoria > -1) 
	{
		char* linea = readline("\nIngresá una función: ");  			
		identificar_linea(linea);
		free(linea);
	}

	log_destroy(logger);
	config_destroy(kernel_config);
	// TODO: sem_destroy (TODOS LOS MODULOS)
}

void identificar_linea(char* linea)
{
	char** tokens = string_split(linea, " "); 
	t_funcion funcion = pedir_enum_funcion(tokens);
	switch (funcion)
	{
		case INICIAR_PROCESO:
			char* path = strdup(tokens[1]);
			int size = atoi(tokens[2]);
			int prioridad = -1;

			if (algoritmo_plani == PRIORIDADES)
			{
				prioridad = atoi(tokens[3]);
			}
			
			log_trace(logger, "INICIAR PROCESO %s", path);
			crear_proceso(path, size, prioridad);
			break;
		case FINALIZAR_PROCESO:
			int pid = atoi(tokens[1]);
			t_pcb* pcb = buscar_pcb_por_pid(pid);
			log_trace(logger, "FINALIZAR PROCESO %d", pid);
			if(pcb->estado != FIN)
			{
				pcb->fin_por_consola = true;
				finalizar_proceso(pcb, "Finalizado por consola."); // TODO: no anda detener-crear-finalizar-iniciar 
			}
			break;
		case DETENER_PLANIFICACION:
			log_info(logger, "PAUSA DE PLANIFICACION");
			detener_planificacion();
			break;
		case INICIAR_PLANIFICACION:
			log_trace(logger, "INICIO DE PLANIFICACION");
			iniciar_planificacion();
			break;
		case MULTIPROGRAMACION:
			int nuevo_grado = atoi(tokens[1]);
			log_trace(logger, "Grado Anterior: %d - Grado Actual: %d", multiprogramacion, nuevo_grado);
			crear_nuevo_semaforo_multiprog(nuevo_grado);
			break;
		case PROCESO_ESTADO:
			listar_estados();
			break;
		default:
			log_info(logger, "No se reconoce el comando");
			break;						
	}
	free_strv(tokens);
}

int pedir_enum_funcion(char** sublinea)
{
	if(strcmp(sublinea[0], "INICIAR_PROCESO") == 0)
		return INICIAR_PROCESO;
	else if(strcmp(sublinea[0], "FINALIZAR_PROCESO") == 0)
		return FINALIZAR_PROCESO;
	else if(strcmp(sublinea[0], "DETENER_PLANIFICACION") == 0)		
		return DETENER_PLANIFICACION;
	else if(strcmp(sublinea[0], "INICIAR_PLANIFICACION") == 0)
		return INICIAR_PLANIFICACION;
	else if(strcmp(sublinea[0], "MULTIPROGRAMACION") == 0)
		return MULTIPROGRAMACION;
	else if(strcmp(sublinea[0], "PROCESO_ESTADO") == 0)
		return PROCESO_ESTADO;
	else
		return -1;
}

void initialize_sockets()
{
	char* path = "/home/utnso/tp-2023-2c-Fulanitos/config/ip.config";
	socket_memoria = start_client_module("MEMORIA", path);
	log_info(logger, "Conexion con servidor MEMORIA en socket %d", socket_memoria);
	
	socket_cpu_interrupt = start_client_module("CPU_INTERRUPT", path);
	log_info(logger, "Conexion con servidor CPU INTERRUPT en socket %d", socket_cpu_interrupt);
	
	socket_cpu_dispatch = start_client_module("CPU_DISPATCH", path);
	log_info(logger, "Conexion con servidor CPU DISPATCH en socket %d", socket_cpu_dispatch);
}


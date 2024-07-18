#include "fileSystem.h"
#define IP_CONFIG_PATH "/home/utnso/tp-2023-2c-Fulanitos/config/ip.config"

uint32_t ultimo_bloque = UINT32_MAX;
char dato = '\0';
uint32_t bloque_libre_fat = 0;

int main(int argc, char **argv) 
{
	initialize_setup(argv[1], "fileSystem", &logger, &fileSystem_config);
	initialize_fsfiles();
	initialize_sockets();
	
	// log_info(logger, "Esperando nueva petición...");
	// int socket_cliente = accept(socket_filesystem, NULL, NULL);
	// log_info(logger, "Nuevo cliente aceptado. [socket %d]", socket_cliente);
	// atender_cliente(socket_cliente);

	while(1)
	{
		log_info(logger, "Esperando nueva petición...");
		int socket_cliente = accept(socket_filesystem, NULL, NULL);
		log_info(logger, "Nuevo cliente aceptado. [socket %d]", socket_cliente);

		pthread_t hilo_peticion;
		int resultado = pthread_create(&hilo_peticion, NULL, atender_cliente, (void*)&socket_cliente);

		if(resultado != 0)
		{
			log_error(logger, "Error al crear hilo petición.");
			exit(EXIT_FAILURE);
		}
		pthread_detach(hilo_peticion);		 
	}
	
	liberar_estructuras();
	return EXIT_SUCCESS;
}

void initialize_sockets() 
{
	char* path = "/home/utnso/tp-2023-2c-Fulanitos/config/ip.config";
	socket_filesystem = start_server_module("FILESYSTEM", path);
	log_info(logger, "Creación de servidor File System. [socket %d]", socket_filesystem);
}

void* atender_cliente(void* arg) 
{	
	int socket_cliente = *(int*)arg;
	log_debug(logger, "Atiendo cliente %d", socket_cliente);
	op_code codigo = recibir_codigo_operacion(socket_cliente);
	int pid = 0;
		size_t largo_nombre = 0;
		char* archivo = NULL;
		uint32_t dir_fisica = 0;
		uint32_t puntero = 0;
		void* contenido = NULL;
		uint32_t nro_bloque;		
		int pagina = 0;
		
	switch(codigo) 
	{
		case HANDSHAKE:
		{
			break;
		}
		case CREATE:
		{
			log_debug(logger, "Entro a create");
			recv(socket_cliente, &largo_nombre, sizeof(size_t), MSG_WAITALL);

			archivo = malloc(largo_nombre);
			recv(socket_cliente, archivo, largo_nombre, MSG_WAITALL);
			
			log_info(logger, "“Crear Archivo: %s.”", archivo);
			crear_fcb(archivo); 
			send_op_code(socket_cliente, OK);
			break;
		}
		case OPEN:
		{
			recv(socket_cliente, &largo_nombre, sizeof(size_t), MSG_WAITALL);

			archivo = malloc(largo_nombre);
			recv(socket_cliente, archivo, largo_nombre, MSG_WAITALL);

			log_info(logger, "“Abrir Archivo: %s.”", archivo);
			int tamanio_archivo = verificar_existencia_fcb(archivo);
			if(tamanio_archivo != -1)
			{
				t_paquete* p = crear_paquete_codigo_operacion(OK);
				add_to_buffer(p->buffer, &tamanio_archivo, sizeof(int));
				enviar_paquete(p, socket_cliente);
			} 
			else send_op_code(socket_cliente, NO_SUCH_FILE);
			log_info(logger, "tamanio:%d", tamanio_archivo);
			free(archivo);
			break; 
		}
 		case READ:
		{
			log_debug(logger, "entro a read");
			recv(socket_cliente, &pid, sizeof(int), MSG_WAITALL);
			recv(socket_cliente, &largo_nombre, sizeof(size_t), MSG_WAITALL);
			archivo = malloc(largo_nombre);
			recv(socket_cliente, archivo, largo_nombre, MSG_WAITALL);
			recv(socket_cliente, &puntero, sizeof(uint32_t), MSG_WAITALL);
			recv(socket_cliente, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
			recv(socket_cliente, &pagina, sizeof(int), MSG_WAITALL);		
		
			log_info(logger, "“Leer Archivo: %s - Puntero: %d - Memoria: %d.”", archivo, puntero, dir_fisica);
			contenido = malloc(tam_bloques);
			leer_bloque(puntero, contenido, archivo); 
			log_debug(logger, "leyo bloque");

			t_paquete* paquete = crear_paquete_codigo_operacion(WRITE);
			add_to_buffer(paquete->buffer, &pid, sizeof(int));
			add_to_buffer(paquete->buffer, &dir_fisica, sizeof(uint32_t));
			add_to_buffer(paquete->buffer, contenido, tam_bloques);
			add_to_buffer(paquete->buffer, &pagina, sizeof(int));
			enviar_paquete(paquete, socket_memoria);
			free(contenido);
			free(archivo);

			validar_codigo_recibido(socket_memoria, OK, logger);
			send_op_code(socket_cliente, OK);
			break;
		}
		case WRITE:
		{
			recv(socket_cliente, &pid, sizeof(int), MSG_WAITALL);
			recv(socket_cliente, &largo_nombre, sizeof(size_t), MSG_WAITALL);
			archivo = malloc(largo_nombre);
			recv(socket_cliente, archivo, largo_nombre, MSG_WAITALL);
			recv(socket_cliente, &puntero, sizeof(uint32_t), MSG_WAITALL);
			recv(socket_cliente, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
			recv(socket_cliente, &pagina, sizeof(int), MSG_WAITALL);		
		
			log_info(logger, "“Escribir Archivo: %s - Puntero: %d - Memoria: %d.”", archivo, puntero, dir_fisica);

			t_paquete* paquete = crear_paquete_codigo_operacion(READ);
			add_to_buffer(paquete->buffer, &pid, sizeof(int));
			add_to_buffer(paquete->buffer, &dir_fisica, sizeof(uint32_t));
			add_to_buffer(paquete->buffer, &pagina, sizeof(int));
			enviar_paquete(paquete, socket_memoria);
			log_debug(logger,"Voy a memoria");
			contenido = malloc(tam_bloques);
			recv(socket_memoria, &contenido, tam_bloques, MSG_WAITALL);
			log_debug(logger,"Recibi contenido");
			escribir_bloque(puntero, contenido, archivo);
			log_debug(logger,"Escribo contenido");

			send_op_code(socket_cliente, OK);

			free(archivo);
			free(contenido);
			break;
		}
		case TRUNCATE:
		{
			uint32_t tamanio_truncado = 0;
			recv(socket_cliente, &largo_nombre, sizeof(size_t), MSG_WAITALL);
			archivo = malloc(largo_nombre);
			recv(socket_cliente, archivo, largo_nombre, MSG_WAITALL);
			recv(socket_cliente, &tamanio_truncado, sizeof(uint32_t), MSG_WAITALL);

			actualizar_tamanio_archivo(archivo, tamanio_truncado); 
			log_info(logger, "“Truncar Archivo: %s - Tamaño: %d.”", archivo, tamanio_truncado);
			send_op_code(socket_cliente, OK);
			
			free(archivo);
			break; 
		}
		case PROCESS_STARTED:
		{
			int bloques_solicitados = 0;
			recv(socket_cliente, &bloques_solicitados, sizeof(int), MSG_WAITALL);
			uint32_t* bloques_reservados_swap = malloc(bloques_solicitados * (sizeof(uint32_t)));
			reservar_bloques_de_swap(bloques_solicitados, bloques_reservados_swap);

			send(socket_cliente, bloques_reservados_swap, bloques_solicitados * sizeof(uint32_t), 0);
			free(bloques_reservados_swap);
			break;
		}
		case PROCESS_FINISHED:
		{
			int n_bloques = 0;

			recv(socket_cliente, &n_bloques, sizeof(int), 0);
			uint32_t* bloques_swap_a_liberar = malloc(n_bloques * sizeof(uint32_t));
			recv(socket_cliente, bloques_swap_a_liberar, n_bloques * sizeof(uint32_t), 0);
			liberar_bloques_swap(bloques_swap_a_liberar, n_bloques);

			send_op_code(socket_cliente, OK);
			log_debug(logger, "Swap liberada");
			break;
		}
		case ESCRIBIR_SWAP:
		{
			recv(socket_cliente, &nro_bloque, sizeof(uint32_t), MSG_WAITALL);
			contenido = malloc(tam_bloques);
			recv(socket_cliente, contenido, tam_bloques, MSG_WAITALL);

			escribir_bloque_swap(nro_bloque, contenido);
			free(contenido);
		}
		case LEER_SWAP:
		{
			log_debug(logger, "entra a leer bloque de swap");
			recv(socket_cliente, &nro_bloque, sizeof(uint32_t), MSG_WAITALL);
			
			contenido = malloc(tam_bloques);
			log_debug(logger, "lee bloque");
			leer_bloque_swap(nro_bloque, contenido);
			log_debug(logger, "leyo bloque");

			send(socket_cliente, contenido, tam_bloques, 0);
			free(contenido);
			break;
		}		
		default:
		{
			if(codigo == -1)
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
	pthread_exit(NULL);
}

void crear_fcb(char* nombre_archivo)
{	
	t_fcb* fcb = malloc(sizeof(t_fcb));
	fcb->nombre_archivo = nombre_archivo;
	char* path_fcb = concatenar_archivo_fcb_con_carpeta(nombre_archivo);
	fcb->tamanio_archivo = 0;
	fcb->bloque_inicial = -1;

	FILE* fcb_file = fopen(path_fcb, "w+");
	if (fcb_file == NULL)
	{
        log_error(logger, "No se pudo crear el archivo");
        exit(EXIT_FAILURE);
    }
	fclose(fcb_file);

	// TODO: caso ya existe archivo

	t_config* config_fcb = config_create(path_fcb);		
	char* tamanio_archivo_string = string_itoa(fcb->tamanio_archivo);
	char* bloque_inicial_string = string_itoa(fcb->bloque_inicial);
	log_debug(logger, "Bloque inicial %s", bloque_inicial_string);
	config_set_value(config_fcb, "NOMBRE_ARCHIVO", fcb->nombre_archivo);
	config_set_value(config_fcb, "TAMANIO_ARCHIVO", tamanio_archivo_string);
	config_set_value(config_fcb, "BLOQUE_INICIAL", bloque_inicial_string);
	config_save_in_file(config_fcb, path_fcb);
	log_debug(logger, "Bloque inicial %d", config_get_int_value(config_fcb, "BLOQUE_INICIAL"));

	config_destroy(config_fcb);
	free(fcb);
}

int verificar_existencia_fcb(char* nombre_archivo) 
{
    char* path_fcb = concatenar_archivo_fcb_con_carpeta(nombre_archivo);
    FILE* archivo = fopen(path_fcb, "r");

	if(archivo != NULL) 
	{
		fclose(archivo);
		t_config* config_fcb_archivo = config_create(path_fcb);
		int tamanio = config_get_int_value(config_fcb_archivo, "TAMANIO_ARCHIVO");
		config_destroy(config_fcb_archivo);
		return tamanio;
	} 
	else return -1;
}

uint32_t buscar_en_fat(int bloque_inicial, uint32_t bloque_buscado) // A partir de bloque inicial (bloque 0 de archivo, bloque n de archivo de bloques/fat), recorre la FAT hasta encontrar el bloque buscado del archivo
{
	if(bloque_buscado <= 0){
		return bloque_inicial;
	} else {
		uint32_t bloque_siguiente = bloque_inicial;
		for (uint32_t bloque = 0; bloque < (bloque_buscado-1); bloque++)
		{   
			bloque_siguiente = leer_entrada_fat(bloque_siguiente);
		}
		return bloque_siguiente;
	}
}

uint32_t leer_entrada_fat(uint32_t entrada) // Devuelve contenido de entrada (bloque siguiente)
{
	uint32_t bloque_siguiente = 0;
	fseek(fat_file, entrada * sizeof(uint32_t), SEEK_SET);
	fread(&bloque_siguiente, sizeof(uint32_t), 1, fat_file);
	return bloque_siguiente;
}

void leer_bloque(uint32_t ptr, void* contenido, char* archivo)
{
	uint32_t bloque_archivo = ptr / tam_bloques; 

	if(bloque_archivo == 0); 
	else bloque_archivo = buscar_en_fat(obtener_bloque_inicial(archivo), bloque_archivo - 1); // bloque archivo a bloque fat
	
    fseek(bloques_file, bloque_archivo * tam_bloques, SEEK_SET);
    fread(contenido, tam_bloques, 1, bloques_file);
	fseek(bloques_file, 0, SEEK_SET);

	log_debug(logger, "contenido:%d",*(uint32_t*) contenido);
}

void escribir_bloque(uint32_t ptr, void* contenido, char* archivo)
{
	uint32_t bloque_archivo = ptr / tam_bloques; 
	log_debug(logger, "Entro a escribir bloque");

	if(bloque_archivo == 0);
	else bloque_archivo = buscar_en_fat(obtener_bloque_inicial(archivo), bloque_archivo - 1); // bloque archivo a bloque fat

	log_debug(logger, "Bloque encontrado");
	escribir_archivo_de_bloques(bloque_archivo * tam_bloques, contenido);
	log_debug(logger, "Bloque escrito");
}

void actualizar_tamanio_archivo(char* nombre_archivo, int tamanio_a_actualizar)
{
	t_fcb* fcb = obtener_fcb(nombre_archivo);
	uint32_t bloques_actuales = fcb->tamanio_archivo / tam_bloques;	
	uint32_t bloques_nuevos = tamanio_a_actualizar / tam_bloques; 

	if(bloques_actuales < bloques_nuevos) asignar_bloques_en_fat(fcb, bloques_nuevos - bloques_actuales);
	else liberar_bloques_en_fat(fcb, bloques_actuales - bloques_nuevos, bloques_actuales);

	fcb->tamanio_archivo = tamanio_a_actualizar;
	actualizar_tamanio_fcb(fcb);

	liberar_fcb(fcb);
}

void liberar_fcb(t_fcb* fcb)
{
	free(fcb->nombre_archivo);
	free(fcb);
}

t_fcb* obtener_fcb(char* nombre_archivo)
{
	char* fcb_path = concatenar_archivo_fcb_con_carpeta(nombre_archivo);
	FILE* fcb_file = fopen(fcb_path, "r");
	if (fcb_file == NULL)
	{
        log_error(logger, "No se pudo abrir el archivo FCB.");
        exit(EXIT_FAILURE);
    }
	fclose(fcb_file);

	t_config* config_fcb = config_create(fcb_path);	

	t_fcb* fcb = malloc(sizeof(t_fcb));
	char* nombre = config_get_string_value(config_fcb, "NOMBRE_ARCHIVO");
	fcb->nombre_archivo = string_duplicate(nombre);
	fcb->bloque_inicial = config_get_int_value(config_fcb, "BLOQUE_INICIAL");
	fcb->tamanio_archivo = config_get_int_value(config_fcb, "TAMANIO_ARCHIVO");

	config_destroy(config_fcb);
	free(fcb_path);
	return fcb;
}

int obtener_bloque_inicial(char* nombre_archivo)
{
	char* fcb_path = concatenar_archivo_fcb_con_carpeta(nombre_archivo);
	FILE* fcb_file = fopen(fcb_path, "r");
	if (fcb_file == NULL)
	{
        log_error(logger, "No se pudo abrir el archivo FCB.");
        exit(EXIT_FAILURE);
    }
	fclose(fcb_file);

	t_config* config_fcb = config_create(fcb_path);	
	int bloque_inicial = config_get_int_value(config_fcb, "BLOQUE_INICIAL");
	config_destroy(config_fcb);
	free(fcb_path);
	return bloque_inicial;
}

void asignar_bloques_en_fat(t_fcb* fcb, int bloques_a_asignar)
{
	int bloque = fcb->bloque_inicial; 
	if (bloque > 0)
	{
		do bloque = leer_entrada_fat((uint32_t)bloque); // chequear que retorne uint32max
		while (bloque != ultimo_bloque);
	}
	int bloque_anterior = bloque;
	for (bloque = 1; bloque <= cant_bloques_fat; bloque++)
	{
		if (!bitarray_test_bit(bloques_libres_fat, bloque))
		{
			bitarray_set_bit(bloques_libres_fat, bloque);
			escribir_archivo_de_bloques(bloque * tam_bloques, &dato);

			if (fcb->bloque_inicial == -1) 
			{	
				fcb->bloque_inicial = bloque;
				actualizar_bloque_inicial_fcb(fcb);
				bloque_anterior = bloque;
			}
			else
			{
				fseek(fat_file, bloque_anterior * sizeof(uint32_t), SEEK_SET);
				if(bloques_a_asignar == 1) // cuando llego a ultimo bloque a asignar
				{
					uint32_t bloque_uint32 = (uint32_t)bloque;
					fwrite(&bloque_uint32, sizeof(uint32_t), 1, fat_file); // engancha bloque anterior y nuevo

					fseek(fat_file, bloque * sizeof(uint32_t), SEEK_SET);
					fwrite(&ultimo_bloque, sizeof(uint32_t), 1, fat_file); // asigna fin de archivo
				}
				else 
				{
					uint32_t bloque_uint32 = (uint32_t)bloque;
					fwrite(&bloque_uint32, sizeof(uint32_t), 1, fat_file); // engancha bloque anterior y nuevo
					bloque_anterior = bloque;
				}
			}
			bloques_a_asignar--;
		}
		if (bloques_a_asignar == 0) 
		{
			fseek(fat_file, 0, SEEK_SET);
			break;
		}
	}
	if (bloques_a_asignar > 0) 
	{
		log_error(logger, "No se pudieron asignar los bloques necesarios. FAT llena.");
		exit(EXIT_FAILURE);
	}
}

void liberar_bloques_en_fat(t_fcb* fcb, int bloques_a_liberar, uint32_t bloques_actuales)
{ 
	uint32_t bloque_inicio = buscar_en_fat(fcb->bloque_inicial, bloques_actuales - bloques_a_liberar - 1); 
	uint32_t bloque = bloque_inicio;
	
	if((bloques_a_liberar - bloques_actuales) == 0)
	{
		fseek(fat_file, bloque_inicio * sizeof(uint32_t), SEEK_SET);
		fwrite(&bloque_libre_fat, sizeof(uint32_t), 1, fat_file);
		fcb->bloque_inicial = -1;
		actualizar_bloque_inicial_fcb(fcb);
	}
	else
	{
		fseek(fat_file, bloque_inicio * sizeof(uint32_t), SEEK_SET);
		fwrite(&ultimo_bloque, sizeof(uint32_t), 1, fat_file);
	}

	for (int i = 0; i < bloques_a_liberar - 1; i++)
	{
		bloque = leer_entrada_fat(bloque);
		fseek(fat_file, bloque * sizeof(uint32_t), SEEK_SET);
		fwrite(&bloque_libre_fat, sizeof(uint32_t), 1, fat_file);
		bitarray_clean_bit(bloques_libres_fat, bloque);
		escribir_archivo_de_bloques(bloque * tam_bloques, &dato);
	}
}

void actualizar_bloque_inicial_fcb(t_fcb* fcb)
{	
	t_config* config_fcb = config_create(concatenar_archivo_fcb_con_carpeta(fcb->nombre_archivo));
	char* bloque = string_itoa(fcb->bloque_inicial);
	config_set_value(config_fcb, "BLOQUE_INICIAL", bloque);
	config_save_in_file(config_fcb, concatenar_archivo_fcb_con_carpeta(fcb->nombre_archivo));
	config_destroy(config_fcb);
	free(bloque);
}

void actualizar_tamanio_fcb(t_fcb* fcb)
{
	t_config* config_fcb = config_create(concatenar_archivo_fcb_con_carpeta(fcb->nombre_archivo));
	char* tamanio = string_itoa(fcb->tamanio_archivo);
	config_set_value(config_fcb, "TAMANIO_ARCHIVO", tamanio);
	config_save_in_file(config_fcb, concatenar_archivo_fcb_con_carpeta(fcb->nombre_archivo));
	config_destroy(config_fcb);
	free(tamanio);
}

void liberar_estructuras() 
{
    free(tabla_fat);
	fclose(bloques_file);
	fclose(fat_file);
	config_destroy(fileSystem_config);
	persistir_bitarray("/home/utnso/tp-2023-2c-Fulanitos/fileSystem/src/bitarray_fat.dat", bloques_libres_fat, cant_bloques_fat);
	persistir_bitarray("/home/utnso/tp-2023-2c-Fulanitos/fileSystem/src/bitarray_swap.dat", bloques_libres_swap, cant_total_bloques - cant_bloques_fat);
	bitarray_destroy(bloques_libres_swap);
	log_destroy(logger);
	bitarray_destroy(bloques_libres_fat);
}

void persistir_bitarray(char* path, t_bitarray* puntero_bitarray_a_persistir, int cant_bloques)
{
	FILE* bitarray_file = fopen(path, "wb+");
    if (bitarray_file != NULL)
	{
		fseek(bitarray_file, 0, SEEK_SET);
        fwrite(puntero_bitarray_a_persistir->bitarray, 1, cant_bloques/8, bitarray_file);
        fclose(bitarray_file);
	}
}

char* concatenar_archivo_fcb_con_carpeta(char* nombre_archivo)
{
	char* extension = "fcb";
	size_t long_path = strlen(path_carpeta_fcbs) + 1 + strlen(nombre_archivo) + 1 + strlen(extension) + 1;
	char* path = malloc(long_path);

	sprintf(path, "%s/%s.%s", path_carpeta_fcbs, nombre_archivo, extension);
	return path;
}

void escribir_archivo_de_bloques(uint32_t ptr, void* contenido)
{
    fseek(bloques_file, ptr, SEEK_SET);
	fwrite(contenido, 1, tam_bloques, bloques_file);
	fseek(bloques_file, 0, SEEK_SET);
}

void reservar_bloques_de_swap(int cant_bloques_swap_a_reservar, uint32_t* bloques_reservados_swap)
{
	uint32_t bloque_leido;
	int indice_bloque_reservado = 0;
	char dato = '\0';
	for (bloque_leido = 0; bloque_leido <= cant_bloques_swap; bloque_leido++)
	{
		if (!bitarray_test_bit(bloques_libres_swap, bloque_leido))
		{
			//log_debug(logger, "Bloque Swap %d reservado", bloque_leido);
			bitarray_set_bit(bloques_libres_swap, bloque_leido);
			bloques_reservados_swap[indice_bloque_reservado] = bloque_leido;
			cant_bloques_swap_a_reservar--;
			indice_bloque_reservado++;
			escribir_archivo_de_bloques(bloque_leido * tam_bloques, &dato);
			//mem_hexdump((void*)puntero_bitarray, bitarray_get_max_bit(bloques_libres_swap));
		}
		if (cant_bloques_swap_a_reservar == 0) break;
	}

	if (cant_bloques_swap_a_reservar > 0) 
	{
		log_error(logger, "No se pudieron asignar los bloques necesarios. SWAP llena.");
		exit(EXIT_FAILURE);
	}
}

void liberar_bloques_swap(uint32_t* bloques_swap_a_liberar, int n_bloques)
{
	for(int i = 0; i < n_bloques; i++)
		bitarray_clean_bit(bloques_libres_swap, bloques_swap_a_liberar[i]);
}

void initialize_fsfiles()
{
	cant_total_bloques = config_get_int_value(fileSystem_config, "CANT_BLOQUES_TOTAL");
	cant_bloques_swap = config_get_int_value(fileSystem_config, "CANT_BLOQUES_SWAP");

	start_tabla_fat();
 	start_archivo_de_bloques();
	start_carpeta_fcb();
}

void start_tabla_fat()
{	
	cant_bloques_fat = cant_total_bloques - cant_bloques_swap;
	tam_tabla_fat = (cant_bloques_fat) * sizeof(uint32_t);
	comienzo_particion_fat = sizeof(uint32_t) * cant_bloques_swap + 1;
	tabla_fat = malloc(tam_tabla_fat);

	char* path_fat = config_get_string_value(fileSystem_config, "PATH_FAT");

	fat_file = fopen(path_fat, "rb+");

	if (fat_file == NULL) 
	{	
		size_t tamanio_bitmap_fat = cant_bloques_fat / 8;
		puntero_bitarray_fat = malloc(tamanio_bitmap_fat);
		bloques_libres_fat = bitarray_create_with_mode(puntero_bitarray_fat, tamanio_bitmap_fat, MSB_FIRST);
		memset(puntero_bitarray_fat, 0, tamanio_bitmap_fat);
		//mem_hexdump((void*)puntero_bitarray_fat, bitarray_get_max_bit(bloques_libres_fat));
		//log_debug(logger, "Bitmap fat iniciado");
		
		log_info(logger, "Creando tabla FAT");
		fat_file = fopen(path_fat, "wb+");

		for (int i = 0; i < cant_bloques_fat; i++) 
		{
			fseek(fat_file, sizeof(uint32_t), SEEK_CUR);
			fwrite(&bloque_libre_fat, sizeof(uint32_t), 1, fat_file);
		}

		uint32_t bloque_reservado = UINT32_MAX;
		fseek(fat_file, 0, SEEK_SET);
		fwrite(&bloque_reservado, sizeof(uint32_t), 1, fat_file);
		bitarray_set_bit(bloques_libres_fat, 0);
		//log_debug(logger, "Bloque de fat reservado");
		//mem_hexdump((void*)puntero_bitarray_fat, bitarray_get_max_bit(bloques_libres_fat));
		//log_debug(logger, "tam bitarray fat %d", bitarray_get_max_bit(bloques_libres_fat));
		// log_debug(logger, "tam bitarray fat %d", tamanio_bitmap_fat);
		// log_debug(logger, "%d", bitarray_test_bit(bloques_libres_fat, 0));
		// log_debug(logger, "%d", bitarray_test_bit(bloques_libres_fat, 1));
	} 
	else 
	{
		log_info(logger, "Se encontró una tabla FAT");		
		crear_bitarray_desde_archivo(&bloques_libres_fat, "/home/utnso/tp-2023-2c-Fulanitos/fileSystem/src/bitarray_fat.dat", cant_bloques_fat);
		//log_debug(logger, "tam bitarray fat %d", bitarray_get_max_bit(bloques_libres_fat));
		//log_debug(logger, "%d", bitarray_test_bit(bloques_libres_fat, 0));
		//log_debug(logger, "%d", bitarray_test_bit(bloques_libres_fat, 1));
	}

 	// uint32_t valor1 = 2;
	// uint32_t valor2 = 6;
	// uint32_t valor3 = 8;

	// fseek(fat_file, sizeof(uint32_t)* 3, SEEK_SET);
	// fwrite(&valor1, sizeof(uint32_t), 1, fat_file);
	// fseek(fat_file, sizeof(uint32_t)* 2, SEEK_SET);
	// fwrite(&valor2, sizeof(uint32_t), 1, fat_file);
	// fseek(fat_file, sizeof(uint32_t)* 6, SEEK_SET);
	// fwrite(&valor3, sizeof(uint32_t), 1, fat_file);
	// fseek(fat_file, 0, SEEK_SET);
	
	// uint32_t bloque = UINT32_MAX;
	// fseek(fat_file, 3 * sizeof(uint32_t), SEEK_SET);
	// fwrite(&bloque, sizeof(uint32_t), 1, fat_file);
	// fseek(fat_file, 0 ,SEEK_SET);
}

void crear_bitarray_desde_archivo(t_bitarray** bitarray, char* path, int tamanio) // TODO:
{
	FILE* bitarray_file = fopen(path, "rb");
	fseek(bitarray_file, 0, SEEK_END);
    size_t size = ftell(bitarray_file);
    fseek(bitarray_file, 0, SEEK_SET);
	char* buffer = malloc(size);
	fread(buffer, size, 1, bitarray_file);
    fclose(bitarray_file);

	*bitarray = bitarray_create_with_mode(buffer, tamanio / 8, MSB_FIRST);
}

void start_archivo_de_bloques() // archivo con todos los bloques del fs (SWAP + FAT)
{	
	char* path_bloques = config_get_string_value(fileSystem_config, "PATH_BLOQUES");
	tam_bloques = config_get_int_value(fileSystem_config, "TAM_BLOQUE");
	
	bloques_file = fopen(path_bloques, "rb+");
	if (bloques_file == NULL) 
	{	
		size_t tamanio_bitmap = cant_bloques_swap / 8; // TODO: contemplar caso que haya que redondear para arriba
		puntero_bitarray = malloc(tamanio_bitmap);
		bloques_libres_swap = bitarray_create_with_mode(puntero_bitarray, tamanio_bitmap, MSB_FIRST);
		memset(puntero_bitarray, 0, tamanio_bitmap);
		//log_debug(logger, "Bitmap swap iniciado");
		//mem_hexdump((void*)puntero_bitarray, bitarray_get_max_bit(bloques_libres_swap));
		log_info(logger, "Creando archivo de bloques");
		bloques_file = fopen(path_bloques, "wb+");
		if (bloques_file == NULL)
		{
        	log_error(logger, "No se pudo abrir o crear el archivo");
        	exit(EXIT_FAILURE);
		}

		fseek(bloques_file, 0, SEEK_SET);
		size_t tamanio_archivo_bloques = cant_total_bloques * tam_bloques;
		char buffer[tamanio_archivo_bloques];
    	memset(buffer, 0, tamanio_archivo_bloques);
		fwrite(buffer, sizeof(char), tamanio_archivo_bloques, bloques_file);
		//log_debug(logger, "Archivo bloques");
		//mem_hexdump((void*)bloques_file, cant_total_bloques);		
		//log_debug(logger, "tam bitarray swap %d", bitarray_get_max_bit(bloques_libres_swap));
		// log_debug(logger, "tam bitarray swap %d", tamanio_bitmap);
		// log_debug(logger, "%d", bitarray_test_bit(bloques_libres_swap, 0));
		// log_debug(logger, "%d", bitarray_test_bit(bloques_libres_swap, 1));
		// log_debug(logger, "%d", bitarray_test_bit(bloques_libres_swap, 2));
    }
	else
	{
		log_info(logger, "Se encontró un archivo de bloques");		
		crear_bitarray_desde_archivo(&bloques_libres_swap, "/home/utnso/tp-2023-2c-Fulanitos/fileSystem/src/bitarray_swap.dat", cant_bloques_swap);
		//log_debug(logger, "tam bitarray bloques %d", bitarray_get_max_bit(bloques_libres_swap));
		//log_debug(logger, "%d", bitarray_test_bit(bloques_libres_swap, 0));
		//log_debug(logger, "%d", bitarray_test_bit(bloques_libres_swap, 1));
		//log_debug(logger, "%d", bitarray_test_bit(bloques_libres_swap, 2));
	}

	// char* contenido = "n";
	// fseek(bloques_file, 0, SEEK_SET);
	// for(int i = 1; i < cant_total_bloques; i++){
	// 	fseek(bloques_file, i * tam_bloques, SEEK_SET);
	// 	fwrite(contenido, tam_bloques, 1, bloques_file);
	// }
}

void start_carpeta_fcb()
{
	path_carpeta_fcbs = config_get_string_value(fileSystem_config, "PATH_FCB");
}

void leer_bloque_swap(uint32_t nro_bloque, void* contenido)
{
	log_info(logger, "Acceso SWAP: %d", nro_bloque);
	fseek(bloques_file, nro_bloque * tam_bloques, SEEK_SET);
	fread(contenido, tam_bloques, 1, bloques_file);
}

void escribir_bloque_swap(uint32_t nro_bloque, void* contenido)
{
	log_info(logger, "Acceso SWAP: %d", nro_bloque);
	fseek(bloques_file, nro_bloque * tam_bloques, SEEK_SET);
	fwrite(contenido, tam_bloques, 1, bloques_file);
}
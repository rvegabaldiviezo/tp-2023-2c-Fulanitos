#include "memoria.h"
#include "parser.h"
#define IP_CONFIG_PATH "/home/utnso/tp-2023-2c-Fulanitos/config/ip.config"

sem_t debug;

int main(int argc, char **argv)
{
    initialize_setup(argv[1], "memoria", &logger, &memoria_config);
    inicializar_estructuras_memoria();  
    initialize_sockets();

    sem_init(&debug, 0, 0);
    pthread_t hilo_cpu;
    int resultado = pthread_create(&hilo_cpu, NULL, (void*)atender_cliente, (void*)(intptr_t)socket_cpu);
    pthread_detach(hilo_cpu);
    if(resultado != 0)
    {
        log_error(logger, "Error al crear hilo petición.");
        exit(EXIT_FAILURE);
    }

    atender_cliente(socket_kernel);

    liberar_setup();
    return EXIT_SUCCESS;
}

void* atender_cliente(int socket)
{
    while(1)
    {
        u_int32_t dir_fisica = 0;
        int pc = 0;
        int pid = 0;
        void* contenido = NULL; 
        int nro_pagina = 0;

        op_code codigo = recibir_codigo_operacion(socket);
        //log_debug(logger, "Codigo %d", codigo);

        switch(codigo)
        {
            case INSTRUCCION:
            {
                recv(socket, &pid, sizeof(int), MSG_WAITALL);
                recv(socket, &pc, sizeof(int), MSG_WAITALL);

                t_instruction* instruccion = obtener_instruccion(pid, pc);
                usleep(retardo_en_ms * 1000);

                t_buffer* buffer = crear_buffer();
                add_to_buffer(buffer, &instruccion->operation, sizeof(t_instruction_type)); 
                
                int size_params = list_size(instruccion->parameters);
                add_to_buffer(buffer, &size_params, sizeof(int)); 

                //log_debug(logger, "%s", instruction_strings[instruccion->operation]);
                for(int i = 0; i < size_params; i++) // Agregar c/u de los params
                {
                    char* parametro = list_get(instruccion->parameters, i);
                    //log_debug(logger, "%s", parametro);
                    size_t largo_parametro = strlen(parametro) + 1;
                    add_to_buffer(buffer, &largo_parametro, sizeof(size_t));
                    add_to_buffer(buffer, parametro, largo_parametro);
                }
                //log_debug(logger, "Buffer size %d", buffer->size);

                send_buffer_complete(socket, buffer);
                destroy_buffer(buffer); 
                break;
            }
            case CREAR_PROCESO:   
            {
                size_t largo_nombre_archivo = 0;
                char* nombre_archivo = NULL;
                int size = 0;

                recv(socket, &largo_nombre_archivo, sizeof(size_t), MSG_WAITALL);
                nombre_archivo = malloc(largo_nombre_archivo * sizeof(char));
                recv(socket, nombre_archivo, largo_nombre_archivo, MSG_WAITALL);
                recv(socket, &size, sizeof(int), MSG_WAITALL);

                crear_proceso(nombre_archivo, size);
                
                break;
            }
            case PROCESS_FINISHED:
            {
                recv(socket, &pid, sizeof(int), 0);
                finalizar_proceso(pid);
                send_op_code(socket, OK);
                break; 
            }
            case SOLICITAR_VALOR:  
            {
                int nro_pagina_a_consultar = 0;
                recv(socket, &pid, sizeof(int), MSG_WAITALL);
                recv(socket, &nro_pagina_a_consultar, sizeof(int), MSG_WAITALL);
                
                int marco_consultado = acceder_a_tp_del_proceso(pid, nro_pagina_a_consultar);
                if(marco_consultado > -1) { send_op_code(socket, OK); send(socket, &marco_consultado, sizeof(int), MSG_WAITALL); }
                else if(marco_consultado == -1) send_op_code(socket, PAGE_FAULT);
                break; 
            }
            case WRITE:
            {
                recv(socket, &pid, sizeof(int), MSG_WAITALL);
                recv(socket, &dir_fisica, sizeof(uint32_t), MSG_WAITALL); 
                contenido = malloc(tam_paginas);
                recv(socket, contenido, sizeof(uint32_t), MSG_WAITALL);
                recv(socket, &nro_pagina, sizeof(int), MSG_WAITALL);

                sem_wait(&mutex_procesos);
                t_proceso* proceso = buscar_proceso_por_pid(pid);
                t_entrada_tp* entrada_tp = list_get(proceso->tp, nro_pagina); 
                entrada_tp->modificado = 1;
                entrada_tp->ultimo_acceso = time(NULL);
                sem_post(&mutex_procesos);
                
                sem_wait(&mutex_memoria);
                memcpy(memoria_usuario + dir_fisica, contenido, tam_paginas); 
                sem_post(&mutex_memoria);
                usleep(tiempo_espera_ms * 1000);

                log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion fisica: %d.", pid, dir_fisica); 

                send_op_code(socket, OK); // TODO: cual es el caso no satisfactorio?

                free(contenido);
                break;
            }
            case READ:
            {
                recv(socket, &pid, sizeof(int), MSG_WAITALL);
                recv(socket, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
                recv(socket, &nro_pagina, sizeof(int), MSG_WAITALL);

                sem_wait(&mutex_procesos);
                t_proceso* proceso = buscar_proceso_por_pid(pid);
                t_entrada_tp* entrada_tp = list_get(proceso->tp, nro_pagina); 
                entrada_tp->ultimo_acceso = time(NULL);
                sem_post(&mutex_procesos);

                contenido = malloc(tam_paginas);
                sem_wait(&mutex_memoria);
                memcpy(contenido, memoria_usuario + dir_fisica, tam_paginas);
                sem_post(&mutex_memoria);
                usleep(tiempo_espera_ms * 1000);

                log_info(logger, "PID: %d - Accion: LEER - Direccion fisica: %d.", pid, dir_fisica);

                send(socket, contenido, sizeof(uint32_t), 0);
                free(contenido);
                break;
            }
            case PAGE_FAULT:
            {
                recv(socket, &pid, sizeof(int), MSG_WAITALL);
                recv(socket, &nro_pagina, sizeof(uint32_t), MSG_WAITALL);
                log_debug(logger, "Trayendo pagina %d para PID %d", nro_pagina, pid);

                enviar_bloque_de_swap_a_leer_fs(pid, nro_pagina); // trae pagina de swap
                log_debug(logger, "envio bloque a leer de swap");

                void* contenido = malloc(tam_paginas);
                
                recv(socket_filesystem, contenido, tam_paginas, MSG_WAITALL);
                close(socket_filesystem);
                log_info(logger, "Conexión con File System %d cerrada.", socket_filesystem);             
	
                sem_wait(&mutex_procesos);
                t_proceso* p = buscar_proceso_por_pid(pid);
                t_entrada_tp* entrada = list_get(p->tp, nro_pagina);
                sem_post(&mutex_procesos);

                traer_pagina_a_memoria(p, nro_pagina, contenido);
                log_info(logger, "SWAP IN - PID: %d - Marco: %d - Page In: %d-%d.", pid, entrada->marco, pid, nro_pagina);

                free(contenido);
                send_op_code(socket, OK);
                break;
            }
            default:
            {
                if(codigo == -1)
                {
                    log_trace(logger, "Error critico. Finalizando ejecucion.");
                    pthread_exit(error);
                }
                else
                {
                    log_trace(logger, "Codigo desconocido.");
                    pthread_exit(error);
                }
            }
        }
    }
    pthread_exit(NULL);
}

void crear_proceso(char* path, int size)
{
    t_list* instrucciones = list_create();
    char* path_concatenado_con_carpeta_instrucciones = concatenar_path_con_carpeta_de_instrucciones(path); // TODO: kernel pasa nombre, no path. hay que entrar a la carpeta y abrir el archivo?
    log_debug(logger, "Path %s", path_concatenado_con_carpeta_instrucciones);
    parsear_archivo(path_concatenado_con_carpeta_instrucciones, instrucciones);
    free(path);

    int bloques_necesarios = size / tam_paginas;
    t_proceso* proceso = malloc(sizeof(t_proceso) + bloques_necesarios * sizeof(uint32_t));
    proceso->id = id;
    id++;
    proceso->instrucciones = instrucciones;
    sem_wait(&mutex_procesos);
    list_add(procesos, proceso);

    proceso->tp = list_create();
    double cociente = size / tam_paginas;
    int n_entradas = redondear_hacia_arriba(cociente);
    for (int i = 0; i < n_entradas; i++) 
    {
        t_entrada_tp* entrada = malloc(sizeof(t_entrada_tp));
        entrada->marco = -1;   
        entrada->presente = 0;  
        entrada->modificado = 0;
        entrada->ultimo_acceso = 0;  
        list_add(proceso->tp, entrada);
    }

    log_info(logger, "Creación de tabla de páginas de PID: %d - Tamaño: %d.", proceso->id, list_size(proceso->tp) * tam_paginas);
    sem_post(&mutex_procesos);

    pedir_swap(proceso, bloques_necesarios); 
}

int acceder_a_tp_del_proceso(int pid, int nro_pagina) 
{
    sem_wait(&mutex_procesos);
    t_proceso* proceso = buscar_proceso_por_pid(pid);
    t_entrada_tp* entrada_tp = list_get(proceso->tp, nro_pagina);

    if (entrada_tp != NULL && entrada_tp->presente == 0)
    {
        sem_post(&mutex_procesos);
        return -1;
    }
    else 
    { 
        log_info(logger, "PID: %d - Pagina: %d - Marco: %d", proceso->id, nro_pagina, entrada_tp->marco); 
        sem_post(&mutex_procesos);
        return entrada_tp->marco; 
    }
    
}

void traer_pagina_a_memoria(t_proceso* proceso, int nro_pagina, void* contenido)
{
    log_debug(logger, "Buscando pagina %d de proceso %d", nro_pagina, proceso->id);
    sem_wait(&mutex_procesos);
    sem_wait(&mutex_memoria);

    t_entrada_tp* pagina = list_get(proceso->tp, nro_pagina);
    int marco = buscar_marco_libre();

    if(marco > -1)
    {
        t_victima* victima = malloc(sizeof(t_victima));

        if(strcmp(algoritmo_reemplazo, "FIFO") == 0)
        {
            victima->pagina_victima = pagina;
            victima->tabla_victima = proceso->tp;
            victima->pid_victima = proceso->id;
            list_add(proximas_victimas, victima);
        }

        pagina->marco = marco;
        log_debug(logger, "Marco %d asignado a proceso %d", marco, proceso->id);
        sem_post(&mutex_procesos);
        escribir_en_memoria(marco, contenido);     
    }
    else
    {
        log_debug(logger, "No encontre marco. Reemplazando.");
        marco = seleccionar_pagina_victima_y_reemplazar(proceso, nro_pagina);
        log_debug(logger, "Marco %d asignado a proceso %d", marco, proceso->id);
        escribir_en_memoria(marco, contenido);
    }

    pagina->presente = 1;
    pagina->ultimo_acceso = time(NULL);

    sem_post(&mutex_memoria);
    sem_post(&mutex_procesos);
}

int seleccionar_pagina_victima_y_reemplazar(t_proceso* proceso, int nro_pagina)
{    
    int nro_pagina_victima = 0;
    void* contenido_victima = malloc(tam_paginas);
    t_entrada_tp* pagina_nueva = list_get(proceso->tp, nro_pagina);

    if(strcmp(algoritmo_reemplazo, "LRU") == 0)
    {
        t_proceso* proceso_victima_lru = NULL;
        t_entrada_tp* pagina_victima_lru = NULL;

        t_proceso* proceso1 = NULL;
        t_entrada_tp* pagina1 = NULL;

        for(int i = 0; i < list_size(procesos); i++)
        {
            proceso1 = list_get(procesos, i);
            t_list* tabla_paginas_proceso1_filtrada = list_filter(proceso1->tp, pagina_presente); 
            if(i == proceso->id) list_remove_element(tabla_paginas_proceso1_filtrada, pagina_nueva);           
            if(list_is_empty(tabla_paginas_proceso1_filtrada)) continue;
            pagina1 = list_get_minimum(tabla_paginas_proceso1_filtrada, comparar_minimo);

            if(i == 0) 
            {
                proceso_victima_lru = proceso1;
                pagina_victima_lru = pagina1;
                continue;
            }

            if(pagina1->ultimo_acceso < pagina_victima_lru->ultimo_acceso)
            {
                proceso_victima_lru = proceso1;
                pagina_victima_lru = pagina1;
            }
        }

        if(pagina_victima_lru->modificado == 1)
        {
            memcpy(contenido_victima, memoria_usuario + (pagina_victima_lru->marco * tam_paginas), tam_paginas);
            mandar_a_escribir_bloques_swap_fs(pagina_victima_lru->nro_bloque, contenido_victima);
            pagina_victima_lru->presente = 0;

            nro_pagina_victima = obtener_nro_pag_en_tp(proceso_victima_lru->tp, pagina_victima_lru);
            log_info(logger, "SWAP OUT - PID: %d - Marco: %d - Page Out: %d-%d.", proceso_victima_lru->id, pagina_victima_lru->marco, proceso_victima_lru->id, nro_pagina_victima);
        }

        log_info(logger, "REEMPLAZO - Marco: %d - Page Out: %d-%d - Page In %d-%d.", pagina_victima_lru->marco, proceso_victima_lru->id, nro_pagina_victima, proceso->id, nro_pagina);

        pagina_nueva->marco = pagina_victima_lru->marco;
        pagina_victima_lru->presente = 0;
        pagina_victima_lru->marco = -1;
        pagina_victima_lru->ultimo_acceso = 0;        
    } 

    else if(strcmp(algoritmo_reemplazo, "FIFO") == 0)
    {
        t_victima* victima = list_get(proximas_victimas, list_size(proximas_victimas) - 1);
        if(victima->pagina_victima->modificado == 1) // TODO: se puede abstraer
        {
            memcpy(contenido_victima, memoria_usuario + (victima->pagina_victima->marco * tam_paginas), tam_paginas);
            mandar_a_escribir_bloques_swap_fs(victima->pagina_victima->nro_bloque, contenido_victima);
            victima->pagina_victima->presente = 0;

            nro_pagina_victima = obtener_nro_pag_en_tp(victima->tabla_victima, victima->pagina_victima);
            log_info(logger, "SWAP OUT - PID: %d - Marco: %d - Page Out: %d-%d.", pid_victima, victima->pagina_victima->marco, pid_victima, nro_pagina_victima);
        }
        
        log_info(logger, "REEMPLAZO - Marco: %d - Page Out: %d-%d - Page In %d-%d.", victima->pagina_victima->marco, pid_victima, nro_pagina_victima, proceso->id, nro_pagina);

        pagina_nueva->marco = victima->pagina_victima->marco;
        victima->pagina_victima->marco = -1;
        victima->pagina_victima->presente = 0;

        t_victima* proxima_victima = malloc(sizeof(t_victima));
        proxima_victima->pagina_victima = pagina_nueva;
        proxima_victima->tabla_victima = proceso->tp;
        proxima_victima->pid_victima = proceso->id;
        list_add(proximas_victimas, proxima_victima);
    } 
    
    return pagina_nueva->marco;
}

int buscar_marco_libre()
{
    log_debug(logger, "Buscando marco libre...");
    for(int i = 0; i < cant_marcos; i++)
    {
        log_debug(logger, "Marco %d contenido %d", i, bitarray_test_bit(marcos_libres, i));
        if(!bitarray_test_bit(marcos_libres, i))
            return i;
    }
    return -1;
}

bool pagina_presente(void* data) 
{
    t_entrada_tp* pagina = (t_entrada_tp*)data;
    return pagina->presente == 1;
}

void* comparar_minimo(void* arg1, void* arg2)
{
    t_entrada_tp* e1 = (t_entrada_tp*) arg1;
    t_entrada_tp* e2 = (t_entrada_tp*) arg2;
    if (e1->ultimo_acceso < e2->ultimo_acceso) return e1;
    else if (e1->ultimo_acceso > e2->ultimo_acceso) return e2;
    return e1;
}

void escribir_en_memoria(int marco, void* contenido)
{
    uint32_t dir_fisica = marco * tam_paginas;
    memcpy(memoria_usuario + dir_fisica, contenido, tam_paginas);
    bitarray_set_bit(marcos_libres, marco);
}

void finalizar_proceso(int pid)
{   
    sem_wait(&mutex_procesos);
    t_proceso* p = buscar_proceso_por_pid(pid);
    log_info(logger, "Destrucción de tabla de páginas de PID: %d - Tamaño: %d", pid, list_size(p->tp) * tam_paginas);
    sem_post(&mutex_procesos);
    destruir_instrucciones(p);
    liberar_tabla(p);
    liberar_swap(p); 
    sem_wait(&mutex_procesos);
    list_remove_element(procesos, p);
    free(p);
    sem_post(&mutex_procesos);
}

void destruir_instrucciones(t_proceso* p)
{
    sem_wait(&mutex_procesos);
    for(int i = 0; i < list_size(p->instrucciones); i++)
        destruir_instruccion(list_get(p->instrucciones, i));
    list_destroy(p->instrucciones);
    sem_post(&mutex_procesos);
}

void liberar_tabla(t_proceso* p)
{
    t_entrada_tp* pagina = NULL;
    sem_wait(&mutex_procesos);

    for(int i = 0; i < list_size(p->tp); i++)
    {
        pagina = list_get(p->tp, i);
        if(pagina->presente) 
        {
            log_debug(logger, "Liberando marco %d de proceso %d", pagina->marco, p->id);
            bitarray_clean_bit(marcos_libres, pagina->marco);
            sem_wait(&mutex_pagina_aux);
            pagina_aux = pagina;
            t_victima* victima = list_remove_by_condition(proximas_victimas, distinta_pagina);
            sem_post(&mutex_pagina_aux);
            if(victima != NULL) 
            {
                free(victima);
            }
        }
        free(pagina);
    }
    
    list_destroy(p->tp);
    sem_post(&mutex_procesos);
}

bool distinta_pagina(void* element)
{
    t_victima* victima = (t_victima*)element;
    return victima->pagina_victima != pagina_aux;
}

void liberar_swap(t_proceso* p)
{
    sem_wait(&mutex_procesos);
    int bloques = contar_bloques(p->bloques); 

	socket_filesystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
	log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_filesystem);

    t_paquete* paquete = crear_paquete_codigo_operacion(PROCESS_FINISHED);
    add_to_buffer(paquete->buffer, &bloques, sizeof(int));
    add_to_buffer(paquete->buffer, p->bloques, bloques * sizeof(uint32_t));
    sem_post(&mutex_procesos);
    enviar_paquete(paquete, socket_filesystem);

	if(!validar_codigo_recibido(socket_filesystem, OK, logger)) 
	{ 
		log_error(logger, "No se pudo liberar la swap asignada al proceso."); 
		exit(EXIT_FAILURE); 
	}

    close(socket_filesystem);
    log_info(logger, "Conexión con File System %d cerrada.", socket_filesystem);
}

t_proceso* buscar_proceso_por_pid(int pid)
{
	pid_comparador = pid;
	t_proceso* p = list_find(procesos, (void*) igual_pid);
	return p;
}

void pedir_swap(t_proceso* proceso, int bloques_necesarios)
{
	socket_filesystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
	log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_filesystem);
    
    t_paquete* paquete = crear_paquete_codigo_operacion(PROCESS_STARTED);
    add_to_buffer(paquete->buffer, &bloques_necesarios, sizeof(int));
    enviar_paquete(paquete, socket_filesystem);

    sem_wait(&mutex_procesos);
    for(int i = 0; i < bloques_necesarios; i++)
    {
        recv(socket_filesystem, &(proceso->bloques[i]), sizeof(uint32_t), MSG_WAITALL);
        t_entrada_tp* entrada = list_get(proceso->tp, i);
        if (entrada != NULL) entrada->nro_bloque = proceso->bloques[i]; 
        else log_error(logger, "Índice fuera de límite. [entrada->bloques]");
        //log_debug(logger, "Entrada asociada a bloque %d", entrada->nro_bloque);
    }
    sem_post(&mutex_procesos);
    
	close(socket_filesystem);
    log_info(logger, "Conexión con File System %d cerrada.", socket_filesystem);
}

char* concatenar_path_con_carpeta_de_instrucciones(char* path)
{
    char* path_carpeta_instrucciones = config_get_string_value(memoria_config, "PATH_INSTRUCCIONES");
    log_debug(logger, "Path carpeta %s", path_carpeta_instrucciones);
    path = string_from_format("%s/%s", path_carpeta_instrucciones, path);
    log_debug(logger, "Path concatenado %s", path);
	return path;
}

t_instruction* obtener_instruccion(int pid, int pc)
{
    sem_wait(&mutex_procesos);
    t_proceso* proceso = buscar_proceso_por_pid(pid);
    t_instruction* instruccion = (t_instruction*)list_get(proceso->instrucciones, pc);
    sem_post(&mutex_procesos);
    return instruccion;
}    

void mandar_a_escribir_bloques_swap_fs(uint32_t nro_bloque, void* contenido)
{
	socket_filesystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
	log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_filesystem);

    t_paquete* paquete = crear_paquete_codigo_operacion(ESCRIBIR_SWAP);
    add_to_buffer(paquete->buffer, &nro_bloque, sizeof(uint32_t));
    add_to_buffer(paquete->buffer, contenido, tam_paginas);
    enviar_paquete(paquete, socket_filesystem);
    free(contenido);

    close(socket_filesystem);
    log_info(logger, "Conexión con File System %d cerrada.", socket_filesystem);
}

void enviar_bloque_de_swap_a_leer_fs(int pid, uint32_t nro_pag)
{
    sem_wait(&mutex_procesos);
    t_proceso* proceso = buscar_proceso_por_pid(pid);
    t_entrada_tp* entrada = list_get(proceso->tp, nro_pag);
    sem_post(&mutex_procesos);

    log_debug(logger, "se levanta conexion de fs");
    socket_filesystem = start_client_module("FILESYSTEM", IP_CONFIG_PATH);
	log_info(logger, "Conexion con servidor FILE SYSTEM en socket %d", socket_filesystem);
    log_debug(logger, "se levanta conexion de fs");

    //log_debug(logger, "Pidiendo bloque %d a FS", entrada->nro_bloque);
    t_paquete* paquete = crear_paquete_codigo_operacion(LEER_SWAP);
    add_to_buffer(paquete->buffer, &entrada->nro_bloque, sizeof(uint32_t));
    enviar_paquete(paquete, socket_filesystem);
}

int obtener_nro_pag_en_tp(t_list* tp, t_entrada_tp* entrada_buscada)
{
    int i;
    t_entrada_tp* entrada_cmp = NULL;
    for(i = 0; i < list_size(tp); i++)
    {
        entrada_cmp = list_get(tp, i);
        if(entrada_cmp->nro_bloque == entrada_buscada->nro_bloque)
            break;
    }
    return i;
}



bool igual_pid(t_proceso* p) 
{ 
    return p->id == pid_comparador; 
}

bool memoria_vacia() 
{
    for(int i = 0; i < cant_marcos; i++) 
    {
        if(bitarray_test_bit(marcos_libres, i))
            return false;
    }
    return true;
}

void initialize_sockets()
{
    char* path = "/home/utnso/tp-2023-2c-Fulanitos/config/ip.config";
    socket_memoria = start_server_module("MEMORIA", path);
    log_info(logger, "Creación de servidor Memoria. [socket %d]", socket_memoria);

    socket_cpu = accept(socket_memoria, NULL, NULL); 
    log_info(logger, "Cliente CPU aceptado en socket %d", socket_cpu);

    socket_kernel = accept(socket_memoria, NULL, NULL); 
	log_info(logger, "Cliente Kernel aceptado en socket %d", socket_kernel);

    validar_codigo_recibido(socket_cpu, HANDSHAKE, logger);
    send(socket_cpu, &tam_paginas, sizeof(int), 0);
}

void inicializar_estructuras_memoria()
{
    tam_memoria = config_get_int_value(memoria_config, "TAM_MEMORIA");
    tam_paginas = config_get_int_value(memoria_config, "TAM_PAGINA");
    tiempo_espera_ms = config_get_int_value(memoria_config, "RETARDO_RESPUESTA");
    algoritmo_reemplazo = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO");
    retardo_en_ms = config_get_int_value(memoria_config, "RETARDO_RESPUESTA");

    procesos = list_create();
    memoria_usuario = malloc(tam_memoria);
    cant_marcos = tam_memoria / tam_paginas;
    proximas_victimas = list_create();
    error = malloc(sizeof(int));   
    *error = -1;
    id = 0;
    pid_comparador = -1;
    
    size_t tamanio_marcos_libres = cant_marcos / 8;
    if(tamanio_marcos_libres == 0) tamanio_marcos_libres = 1;
    char* puntero = malloc(tamanio_marcos_libres);
    marcos_libres = bitarray_create_with_mode(puntero, tamanio_marcos_libres, MSB_FIRST);
    memset(puntero, 0, tamanio_marcos_libres);

    sem_init(&mutex_pid_comparador, 0, 1);
    sem_init(&mutex_memoria, 0, 1);
    sem_init(&mutex_procesos, 0, 1);
    sem_init(&mutex_pagina_aux, 0, 1);
}

void liberar_setup()
{
    log_destroy(logger);
    config_destroy(memoria_config);
    liberar_procesos();
    free(memoria_usuario);
    bitarray_destroy(marcos_libres);
    list_destroy(proximas_victimas);
    sem_destroy(&mutex_pid_comparador);
    sem_destroy(&mutex_memoria);
    sem_destroy(&mutex_procesos);
    sem_destroy(&mutex_pagina_aux);
    return;
}

void liberar_procesos()
{
    for(int i = 0; i < list_size(procesos); i++)
    {
        t_proceso* proceso = list_get(procesos, i);
        liberar_tabla(proceso);
        destruir_instrucciones(proceso);
        free(proceso);
    }
    list_destroy(procesos);
}

void liberar_tablas()
{
    for(int i = 0; i < list_size(procesos); i++)
        liberar_tabla(list_get(procesos, i));
}

#ifndef __SETUP_H
#define __SETUP_H

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
#include "custom_logs.h"
#include "socket.h"
#include "serialization.h"
#include "structures.h"
#include "custom_logs.h"
#include "environment_variables.h"

void initialize_setup(char *config_path, char* modulo, t_log** logger, t_config** config);
void initialize_logger(t_log** logger, char* modulo);
void initialize_config(char *config_path, t_config** config, t_log** logger);
void enviar_respuesta_exitosa(int conexion);

#endif

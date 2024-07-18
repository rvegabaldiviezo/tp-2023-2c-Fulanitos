#ifndef __SOCKET_H
#define __SOCKET_H
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "structures.h"
#include "custom_logs.h"
#include "environment_variables.h"
#include "serialization.h"

int start_client_module(char* module, char* config);
int start_server_module(char* module, char* config);
void handshake(int socket);

#endif

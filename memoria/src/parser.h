#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <shared/serialization.h>
#include <shared/structures.h>
#include <commons/collections/list.h>

void parsear_archivo(char* path, t_list* lista_instrucciones);
void crear_instruccion(FILE* archivo, t_list* lista_instrucciones);
void crear_diccionario();
int count_elements(char** array);

#endif 
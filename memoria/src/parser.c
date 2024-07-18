#include "parser.h"
#include "memoria.h"

extern t_log* logger;
t_dictionary* diccionario = NULL;

const char* instruction_strings[] = {
    "SET",
    "SUM",
    "SUB",
    "JNZ",
    "SLEEP",
    "WAIT",
    "SIGNAL",
    "MOV_IN",
    "MOV_OUT",
    "F_OPEN",
    "F_CLOSE",
    "F_SEEK",
    "F_READ",
    "F_WRITE",
    "F_TRUNCATE",
    "EXIT",
};

void parsear_archivo(char* path, t_list* lista_instrucciones) 
{ 
	FILE* archivo = fopen(path, "r");
	if (archivo == NULL) log_error(logger, "No se pudo abrir el archivo");
	else
	{
		if(diccionario == NULL) crear_diccionario();
		while (!feof(archivo)) crear_instruccion(archivo, lista_instrucciones); // Operacion | Numero de parametros | Parametros

		fclose(archivo);
	}
}

void crear_instruccion(FILE* archivo, t_list* lista_instrucciones)
{   
    t_instruction* instruction = malloc(sizeof(t_instruction));
    t_buffer* buffer_aux = crear_buffer();
	buffer_aux->size = 100;
    buffer_aux->stream = (char*)malloc(buffer_aux->size * sizeof(char));
    fgets(buffer_aux->stream, buffer_aux->size, archivo);

    char *pos;
    if ((pos = strchr(buffer_aux->stream, '\n')) != NULL) *pos = '\0';

    char** words = (char**)malloc(4 * sizeof(char*));
    words = string_split(buffer_aux->stream, " ");
    char* operacion = words[0];

    instruction->operation = (t_instruction_type) dictionary_get(diccionario, operacion);
    //log_trace(logger, "Operacion %s", instruction_strings[instruction->operation]);

    instruction->parameters = list_create();
    int numero_elementos = count_elements(words);
    for (int pos = 1; pos < numero_elementos; pos++) 
	{
        list_add(instruction->parameters, words[pos]); 
        //log_trace(logger, "Parametro %s", words[pos]);
	}
	list_add(lista_instrucciones, instruction);

    destroy_buffer(buffer_aux);
}

void crear_diccionario()
{
    diccionario = dictionary_create();
   	dictionary_put(diccionario, "SET", (void*)((int)SET));
	dictionary_put(diccionario, "SUM", (void*)((int)SUM));
	dictionary_put(diccionario, "SUB", (void*)((int)SUB));
	dictionary_put(diccionario, "JNZ", (void*)((int)JNZ));
	dictionary_put(diccionario, "SLEEP", (void*)((int)SLEEP));
	dictionary_put(diccionario, "WAIT", (void*)((int)WAIT));
	dictionary_put(diccionario, "SIGNAL", (void*)((int)SIGNAL));
	dictionary_put(diccionario, "MOV_IN", (void*)((int)MOV_IN));
	dictionary_put(diccionario, "MOV_OUT", (void*)((int)MOV_OUT));
	dictionary_put(diccionario, "F_OPEN", (void*)((int)F_OPEN));
	dictionary_put(diccionario, "F_CLOSE", (void*)((int)F_CLOSE));
	dictionary_put(diccionario, "F_SEEK", (void*)((int)F_SEEK));
	dictionary_put(diccionario, "F_READ", (void*)((int)F_READ));
	dictionary_put(diccionario, "F_WRITE", (void*)((int)F_WRITE));
	dictionary_put(diccionario, "F_TRUNCATE", (void*)((int)F_TRUNCATE));
	dictionary_put(diccionario, "EXIT", (void*)((int)EXIT));
    dictionary_put(diccionario, "AX", (void*)((int)AX));
	dictionary_put(diccionario, "BX", (void*)((int)BX));
	dictionary_put(diccionario, "CX", (void*)((int)CX));
    dictionary_put(diccionario, "DX", (void*)((int)DX));
}


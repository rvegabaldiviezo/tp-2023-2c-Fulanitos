#ifndef __CUSTOM_LOGS_H
#define __CUSTOM_LOGS_H

#include <stdarg.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include "structures.h"


typedef enum {
	CENTER,
	LEFT,
	RIGHT
} align;

void log_rect(t_log* logger, const char* title, const char* body, ...);
void log_rectangle(t_log* logger, char border, char fill, align align, const char* message, ...);
void log_instructions(t_log* logger, t_list* instructions);

#endif

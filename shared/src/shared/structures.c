#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <time.h>
#include "structures.h"

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

const char* REGISTRO[] = {
    "AX",
	"BX",
	"CX",
	"DX",
};
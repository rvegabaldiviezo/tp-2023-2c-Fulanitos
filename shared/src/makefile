# Variables Globales
RM=rm -rf # Remover iterativamente 
CC=gcc    # Compilar 
CDEBUG = -g -DDEBUG

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
O_SRCS=$(shell find . -iname "*.o" | tr '\n' ' ')
D_SRCS=$(shell find . -iname "*.d" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

ifneq ($(shell id -un),root)
SUDO=sudo
endif

ver:
	@echo "=== Archivos .c:"
	@$(foreach file,$(C_SRCS),printf "\t%s\n" $(file);)
	@echo "=== Archivos .h:"
	@$(foreach file,$(H_SRCS),printf "\t%s\n" $(file);)
	@echo "=== Carpeta build:"
	@if [ -d "build" ]; then ls -1 build/; else echo "La carpeta build no existe"; fi
	@echo "=== Carpeta build/shared/ :"
	@echo "--- Archivos .o:"
	@$(foreach file,$(O_SRCS),printf "\t%s\n" $(file);)
	@echo "--- Archivos .d:"
	@$(foreach file,$(D_SRCS),printf "\t%s\n" $(file);)	

# Clean and compile .so
all: build/libshared.so

build/shared:
	mkdir -p $@

build/libshared.so: build/shared $(OBJS)
	$(CC) -shared -o "$@" $(OBJS)

build/%.o: %.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

# Add debug parameters and compile
debug: CC = $(CDEBUG)
debug: all

# Print all sources
sources:
	@echo $(C_SRCS) $(H_SRCS)

# Clean build files
clean:
	$(RM) build

install: all
	$(SUDO) cp -u build/libshared.so /usr/lib
	$(SUDO) cp --parents -u $(H_SRCS) /usr/include

uninstall:
	$(SUDO) rm -f /usr/lib/libshared.so
	$(SUDO) rm -rf /usr/include/shared

.PHONY: all debug sources clean install uninstall
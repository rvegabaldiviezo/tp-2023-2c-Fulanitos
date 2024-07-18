CYAN=\033[0;36m
RED=\033[0;31m
NC=\033[0m

@lib:
	@echo "=== Se instalo el archivo /usr/lib/libcommons.so: "
	@ls -lh /usr/lib/libcommons.so
	@echo "=== Se instalo el archivo /usr/lib/libshared.so: "
	@ls -lh /usr/lib/libshared.so

@include: 
	@echo "=== Se instalo el Directorio /usr/include/commons: "
	@ls -lh /usr/include/commons
	@echo "=== Se instalo el Directorio /usr/include/shared: "
	@ls -lh /usr/include/shared

info: @lib @include

shared/uninstall:
	cd shared/src && $(MAKE) uninstall && $(MAKE) clean
	@echo " ${RED}================================${NC}"
	@echo "  ${CYAN}Shared uninstall is complete.${NC}"
	@echo " ${RED}================================${NC}"

shared/install:
	cd shared/src && $(MAKE) install
	@echo " ${RED}================================${NC}"
	@echo "  ${CYAN}Shared install is complete.${NC}"
	@echo " ${RED}================================${NC}"

shared: shared/uninstall shared/install
	@echo " ${RED}================================${NC}"
	@echo "  ${CYAN}Shared build is complete.${NC}"
	@echo " ${RED}================================${NC}"

commons: 
	cd scripts && bash commons.sh 
	@echo " ${RED}================================${NC}"
	@echo "  ${CYAN}commons install is complete.${NC}"
	@echo " ${RED}================================${NC}"

all: shared 
	cd scripts && bash buildall.sh
	@echo " ${RED}================================${NC}"
	@echo "  ${CYAN} all modules is complete.${NC}"
	@echo " ${RED}================================${NC}"

delete:
	cd scripts && ./deleteLogsAndBuild.sh 

.PHONY: shared/uninstall shared/install shared commons all kernel cpu fileSystem memoria delete @lib @include info
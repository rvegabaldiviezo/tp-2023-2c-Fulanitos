MODULO=$1
MODULES_PATH="../$MODULO"

CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

# Compilar modulo que se pasa...

echo -e ""
echo -e "========================================================"
echo -e "=========== COMIENZO COMPILACION DE ${CYAN}$MODULO${NC} ==========="
echo -e "========================================================"
echo -e ""

echo "pwd:  $(pwd)"
(cd $MODULES_PATH && make all)
if [ $? -ne 0 ]; then
    echo -e "${RED}Fallo la compilacion del modulo $MODULO${NC}"
    exit 1
fi

echo -e ""
echo -e "--------------{ MODULO ${CYAN}$MODULO${NC} COMPILADO }--------------"
echo -e ""
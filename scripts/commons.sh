#! / Bin / bash
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

echo -e ""
echo -e "========================================================"
echo -e "=========== COMIENZO INSTALACION DE ${CYAN}commons${NC} ============"
echo -e "========================================================"
echo -e ""

rm -rf so-commons-library

git clone https://github.com/sisoputnfrba/so-commons-library.git

if [ $? -ne 0 ]; then
    echo -e "${RED}No se pudo clonar el repositorio de las commons${NC}"
    exit 1
fi

(
    cd so-commons-library &&
    make uninstall
    make install
)

if [ $? -ne 0 ]; then
    echo -e "${RED}Fallo la instalacion de las commons${NC}"
    exit 1
fi

rm -rf so-commons-library

echo "Directorios instalados:"
(ls -la /usr/lib | grep "commons") && (ls -la /usr/include | grep "commons")


echo -e ""
echo -e "--------{ INSTALACION DE ${CYAN}commons${NC} FINALIZADA }----------"
echo -e ""

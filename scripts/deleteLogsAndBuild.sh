#!/bin/bash

cd .. # Salimos de la carpeta de scripts
ruta="./modules"  # Reemplaza con la ruta deseada

# Verificar si la ruta es válida y existe
if [ ! -d "$ruta" ]; then
  echo "La ruta especificada no existe."
  exit 1
fi

# Eliminar los archivos name.log
find "$ruta" -type f -name "*.log" -delete

echo "--> Archivos name.log eliminados correctamente."

# Buscar y eliminar la carpeta "build" y su contenido
find "$ruta" -type d -name "build" -exec rm -r {} +

echo "--> Carpeta 'build' eliminada correctamente."
#!/bin/bash
# Comando simple para compilar el proyecto

gcc -Wall -Wextra -std=c11 -rdynamic $(pkg-config --cflags gtk+-3.0) -o proyecto-4aa proyecto-4aa.c $(pkg-config --libs gtk+-3.0)

if [ $? -eq 0 ]; then
    echo "Compilación exitosa. Ejecutar con: ./proyecto-4aa"
else
    echo "Error en la compilación"
fi




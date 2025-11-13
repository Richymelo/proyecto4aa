# Proyecto 4 - Análisis de Algoritmos
## II Semestre 2025

Programa para análisis de grafos con detección de propiedades hamiltonianas y eulerianas. Genera documentos LaTeX con visualización del grafo.

## Características

- Interfaz gráfica con GTK+ y Glade
- Entrada de grafos dirigidos y no dirigidos (hasta 12 nodos)
- Matriz de adyacencia editable con validación automática
- Guardado y carga de grafos
- Detección de ciclos y rutas hamiltonianas (backtracking)
- Detección de grafos eulerianos y semieulerianos
- Generación automática de documentos LaTeX de alta calidad
- Compilación y visualización automática de PDFs

## Requisitos

- Linux (Ubuntu/Debian recomendado)
- GTK+ 3.0 (`libgtk-3-dev`)
- pkg-config
- Compilador GCC
- LaTeX (texlive-latex-base, texlive-latex-extra)
- Evince (para visualización de PDFs)

## Instalación de Dependencias

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y libgtk-3-dev pkg-config texlive-latex-base texlive-latex-extra texlive-fonts-recommended evince
```

O usar el Makefile:
```bash
make install-deps
```

### Compilación:

**Opción 1: Usando Makefile:**
```bash
make
```

**Opción 2: Comando directo (sin Makefile):**
```bash
gcc -Wall -Wextra -std=c11 $(pkg-config --cflags gtk+-3.0) -o proyecto-4aa proyecto-4aa.c $(pkg-config --libs gtk+-3.0)
```

**Opción 3: Usando el script:**
```bash
chmod +x COMPILAR.sh
./COMPILAR.sh
```

## Uso

1. Ejecutar el programa:
```bash
./proyecto-4aa
```

2. Configurar el grafo:
   - Ingresar el número de nodos (1-12) y presionar "Aplicar"
   - Seleccionar tipo de grafo (Dirigido o No Dirigido)
   - Ingresar las posiciones relativas de cada nodo (x, y)

3. Ingresar la matriz de adyacencia:
   - En la pestaña "Matriz de Adyacencia", ingresar 1 para aristas existentes, 0 para no existentes
   - La diagonal no es editable (no se permiten bucles)
   - Para grafos no dirigidos, la matriz se hace simétrica automáticamente

4. Generar documento LaTeX:
   - Presionar "Generar Documento LaTeX"
   - El programa generará el archivo `.tex`, lo compilará a PDF y lo mostrará en evince

5. Guardar/Cargar grafos:
   - Usar el menú "Archivo" > "Guardar Grafo" o "Cargar Grafo"

## Estructura del Proyecto

- `proyecto-4aa.c` - Código fuente principal
- `proyecto-4aa.glade` - Archivo de interfaz gráfica (Glade)
- `Makefile` - Archivo para compilación
- `README.md` - Este archivo

## Contenido del Documento LaTeX Generado

El documento generado incluye:

1. **Portada** - Identificación del proyecto y grupo
2. **William Rowan Hamilton** - Breve semblanza
3. **Ciclos y Rutas Hamiltonianas** - Explicación teórica
4. **Leonhard Euler** - Breve semblanza
5. **Ciclos y Rutas Eulerianas** - Explicación teórica
6. **Grafo Original** - Visualización del grafo con TikZ
   - Colores según paridad de grados (2 colores para no dirigidos, 4 para dirigidos)
   - Posiciones respetadas con escalado automático
7. **Propiedades del Grafo** - Análisis de:
   - Existencia de ciclos hamiltonianos
   - Existencia de rutas hamiltonianas
   - Propiedades eulerianas (euleriano, semieuleriano, o ninguno)

## Notas

- El archivo Glade es la base de la interfaz gráfica
- Los archivos de grafos se guardan en formato texto simple
- Los archivos `.tex` y `.pdf` se generan en el directorio actual
- El programa requiere que el archivo `proyecto-4aa.glade` esté en el mismo directorio

## Limpieza

Para eliminar archivos generados:
```bash
make clean
```

## Autores

Grupo de Trabajo - II Semestre 2025
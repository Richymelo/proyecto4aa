# Archivos de Grafos Generados

Este directorio contiene **50 grafos distintos** listos para cargar en el programa.

## Formato de Archivo

Cada archivo sigue el siguiente formato:
1. Primera línea: Número de nodos (K)
2. Segunda línea: Tipo de grafo (0 = No dirigido, 1 = Dirigido)
3. K líneas: Matriz de adyacencia (K números por línea, separados por espacios)
4. K líneas: Posiciones x y de cada nodo (dos números por línea)

## Tipos de Grafos Generados

### 1-5: Grafos Completos (grafo_01 a grafo_05)
- Todos los nodos están conectados entre sí
- Tamaños: 3, 4, 5, 6, 7 nodos
- No dirigidos

### 6-10: Grafos Ciclo (grafo_06 a grafo_10)
- Forman un ciclo cerrado
- Tamaños: 3, 4, 5, 6, 7 nodos
- No dirigidos

### 11-15: Grafos Camino (grafo_11 a grafo_15)
- Forman un camino lineal
- Tamaños: 3, 4, 5, 6, 7 nodos
- No dirigidos

### 16-20: Grafos Estrella (grafo_16 a grafo_20)
- Un nodo central conectado a todos los demás
- Tamaños: 4, 5, 6, 7, 8 nodos
- No dirigidos

### 21-23: Grafos Bipartitos (grafo_21 a grafo_23)
- Dos conjuntos de nodos, todos conectados entre conjuntos
- Tamaños: 4, 6, 8 nodos
- No dirigidos

### 24-28: Grafos Aleatorios (grafo_24 a grafo_28)
- Conexiones aleatorias con diferentes densidades
- Tamaños: 5, 6, 7, 8, 9 nodos
- Densidades: 30%, 40%, 50%, 60%, 70%
- No dirigidos

### 29-33: Grafos Eulerianos (grafo_29 a grafo_33)
- Todos los nodos tienen grado par (garantizan ciclo euleriano)
- Tamaños: 4, 5, 6, 7, 8 nodos
- No dirigidos

### 34-38: Grafos Rueda (grafo_34 a grafo_38)
- Ciclo periférico + nodo central conectado a todos
- Tamaños: 4, 5, 6, 7, 8 nodos
- No dirigidos

### 39-43: Grafos Árbol (grafo_39 a grafo_43)
- Estructura de árbol binario (sin ciclos)
- Tamaños: 4, 5, 6, 7, 8 nodos
- No dirigidos

### 44-48: Grafos Dirigidos (grafo_44 a grafo_48)
- Grafos dirigidos aleatorios
- Tamaños: 4, 5, 6, 7, 8 nodos
- Dirigidos

### 49-50: Grafos Aleatorios Grandes (grafo_49 a grafo_50)
- Grafos aleatorios con más nodos
- Tamaños: 9, 10 nodos
- Densidades: 35%, 40%
- No dirigidos

## Uso

Para cargar un grafo en el programa:
1. Ejecuta el programa
2. Haz clic en "Cargar Grafo" o usa el menú
3. Navega al directorio `grafos/`
4. Selecciona el archivo deseado

## Notas

- Todos los grafos tienen posiciones predefinidas para visualización
- Los grafos están numerados del 01 al 50
- Los nombres de archivo indican el tipo y tamaño del grafo
- Los grafos eulerianos están garantizados para tener ciclos eulerianos
- Los grafos dirigidos tienen direccionalidad en sus aristas



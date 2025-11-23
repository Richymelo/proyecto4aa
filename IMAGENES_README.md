# Instrucciones para Obtener las Imágenes de los Matemáticos

Este proyecto requiere imágenes de los siguientes matemáticos para mostrarlas en el PDF generado:

1. **hamilton.jpg** - William Rowan Hamilton
2. **euler.jpg** - Leonhard Euler
3. **hierholzer.jpg** - Carl Hierholzer
4. **fleury.jpg** - Pierre-Henry Fleury

## Método Automático (Recomendado)

Ejecuta el script bash para descargar las imágenes:

```bash
chmod +x download_images.sh
./download_images.sh
```

El script intentará descargar las imágenes desde Wikipedia (dominio público) y las colocarán en el directorio `images/`.

## Método Manual

Si los scripts no funcionan, puedes descargar las imágenes manualmente:

1. Visita Wikipedia y busca cada matemático
2. Descarga las imágenes en formato JPG
3. Coloca las imágenes en el directorio `images/` con los siguientes nombres:
   - `hamilton.jpg`
   - `euler.jpg`
   - `hierholzer.jpg`
   - `fleury.jpg`

4. También puedes copiar las imágenes al directorio raíz del proyecto (donde está `proyecto-4aa.c`)

## Ubicación de las Imágenes

El código LaTeX busca las imágenes en los siguientes directorios (en orden):
- `./` (directorio raíz del proyecto)
- `./images/`
- `./imagenes/`

## Nota

Si alguna imagen no está disponible, el código LaTeX mostrará automáticamente un placeholder con el mensaje "[Imagen no disponible]" en lugar de la imagen faltante.

## Verificación

Después de descargar las imágenes, verifica que existan:

```bash
ls -lh images/*.jpg
```

Deberías ver las 4 imágenes listadas.


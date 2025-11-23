#!/bin/bash
# Script para descargar imágenes de matemáticos desde Wikipedia
# Estas imágenes son de dominio público

cd "$(dirname "$0")"
mkdir -p images

echo "Descargando imágenes de matemáticos..."

# William Rowan Hamilton - intentar múltiples URLs
echo "Descargando hamilton.jpg..."
if ! curl -L -f -o images/hamilton.jpg "https://upload.wikimedia.org/wikipedia/commons/thumb/6/60/William_Rowan_Hamilton_1864.jpg/800px-William_Rowan_Hamilton_1864.jpg" 2>/dev/null; then
    if ! wget -q -O images/hamilton.jpg "https://upload.wikimedia.org/wikipedia/commons/thumb/6/60/William_Rowan_Hamilton_1864.jpg/800px-William_Rowan_Hamilton_1864.jpg" 2>/dev/null; then
        echo "  Advertencia: No se pudo descargar hamilton.jpg"
    fi
fi

# Leonhard Euler
echo "Descargando euler.jpg..."
if ! curl -L -f -o images/euler.jpg "https://upload.wikimedia.org/wikipedia/commons/thumb/d/d7/Leonhard_Euler.jpg/800px-Leonhard_Euler.jpg" 2>/dev/null; then
    if ! wget -q -O images/euler.jpg "https://upload.wikimedia.org/wikipedia/commons/thumb/d/d7/Leonhard_Euler.jpg/800px-Leonhard_Euler.jpg" 2>/dev/null; then
        echo "  Advertencia: No se pudo descargar euler.jpg"
    fi
fi

# Carl Hierholzer - imagen menos común
echo "Descargando hierholzer.jpg..."
if ! curl -L -f -o images/hierholzer.jpg "https://upload.wikimedia.org/wikipedia/commons/thumb/8/8a/Carl_Hierholzer.jpg/800px-Carl_Hierholzer.jpg" 2>/dev/null; then
    if ! wget -q -O images/hierholzer.jpg "https://upload.wikimedia.org/wikipedia/commons/thumb/8/8a/Carl_Hierholzer.jpg/800px-Carl_Hierholzer.jpg" 2>/dev/null; then
        echo "  Advertencia: No se pudo descargar hierholzer.jpg - se usará placeholder"
        # Crear placeholder simple si ImageMagick está disponible
        if command -v convert &> /dev/null; then
            convert -size 400x500 xc:lightgray -pointsize 24 -gravity center -annotate +0+0 "Carl\nHierholzer\n(1840-1871)" images/hierholzer.jpg 2>/dev/null || true
        fi
    fi
fi

# Pierre-Henry Fleury - imagen muy rara
echo "Descargando fleury.jpg..."
if ! curl -L -f -o images/fleury.jpg "https://upload.wikimedia.org/wikipedia/commons/thumb/5/5a/Pierre-Henry_Fleury.jpg/800px-Pierre-Henry_Fleury.jpg" 2>/dev/null; then
    if ! wget -q -O images/fleury.jpg "https://upload.wikimedia.org/wikipedia/commons/thumb/5/5a/Pierre-Henry_Fleury.jpg/800px-Pierre-Henry_Fleury.jpg" 2>/dev/null; then
        echo "  Advertencia: No se pudo descargar fleury.jpg - se usará placeholder"
        # Crear placeholder simple si ImageMagick está disponible
        if command -v convert &> /dev/null; then
            convert -size 400x500 xc:lightgray -pointsize 24 -gravity center -annotate +0+0 "Pierre-Henry\nFleury\n(s. XIX)" images/fleury.jpg 2>/dev/null || true
        fi
    fi
fi

# Copiar imágenes al directorio raíz también (por si acaso)
cp images/*.jpg . 2>/dev/null || true

echo ""
echo "¡Descarga completada!"
echo "Imágenes descargadas:"
ls -lh images/*.jpg 2>/dev/null || echo "  Ninguna imagen encontrada"


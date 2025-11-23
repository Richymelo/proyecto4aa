# Agregar 2 grafos más para completar 50
$rng = New-Object System.Random

for ($idx = 0; $idx -lt 2; $idx++) {
    $n = 9 + $idx
    $densidad = 35 + $idx * 5
    $filename = "grafos\grafo_{0:D2}_aleatorio_{1}_{2}.txt" -f (49 + $idx), $n, $densidad
    
    $content = "$n`n"
    $content += "0`n"
    
    $matriz = New-Object 'int[,]' $n, $n
    
    # Crear matriz aleatoria
    for ($i = 0; $i -lt $n; $i++) {
        for ($j = 0; $j -lt $n; $j++) {
            if ($i -eq $j) {
                $matriz[$i, $j] = 0
            } elseif ($j -gt $i -and $rng.Next(100) -lt $densidad) {
                $matriz[$i, $j] = 1
                $matriz[$j, $i] = 1
            } elseif ($j -lt $i) {
                $matriz[$i, $j] = $matriz[$j, $i]
            } else {
                $matriz[$i, $j] = 0
            }
        }
    }
    
    # Escribir matriz
    for ($i = 0; $i -lt $n; $i++) {
        for ($j = 0; $j -lt $n; $j++) {
            $content += "$($matriz[$i, $j]) "
        }
        $content += "`n"
    }
    
    # Posiciones aleatorias
    for ($i = 0; $i -lt $n; $i++) {
        $x = 200 + $rng.Next(400)
        $y = 150 + $rng.Next(300)
        $content += "$x $y`n"
    }
    
    Set-Content -Path $filename -Value $content
    Write-Host "Creado: $filename"
}

Write-Host "`n¡Completado! Ahora hay 50 grafos en total." -ForegroundColor Green


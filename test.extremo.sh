#!/bin/bash

# Aseguramos que el código esté compilado y actualizado
make

# Creamos el lote de comandos destructivos
cat << 'EOF' > lote_extremo.txt

# 1. prueba de comillas, espacios y pipes
echo      "   'prueba'   "    '   "loca"   ' | cat | wc -c

# 2. Pipeline largo con redirecciones de entrada y salida simultáneas
ls -la | grep "r" | sort | rev | rev | wc -l > "archivo_con_espacios.txt"
cat < "archivo_con_espacios.txt" | grep "0" >> "archivo_con_espacios.txt"

# 3. Built-ins adicionales (cd)
cd /tmp
cd

# 4. Estrés de tabla de jobs, background y bonus (fg, bg)
sleep 2 &
jobs
bg 1
fg 1
sleep 1 &
sleep 2 &
echo "Esperando que terminen los backgrounds..."
sleep 3
jobs

# 5. Fallos intencionales (ver si libera memoria antes de un _exit(127))
comando_que_no_existe "con argumentos" 'y comillas'
echo "hola" | comando_falso_en_pipe | wc -l
< archivo_invento.txt cat 
fg 999
bg 999

# 6. Limpieza final y salida con código específico
rm -f "archivo_con_espacios.txt"
exit 0
EOF

echo -e "Test de la Shell con Valgrind"

# Ejecutamos Valgrind leyendo desde el archivo (sin pmon porque el pause() bloquearía el test)
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./mishell < lote_extremo.txt

# Limpiamos el archivo de prueba
rm -f lote_extremo.txt
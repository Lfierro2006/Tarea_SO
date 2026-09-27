#ifndef PIPES_H
#define PIPES_H

#include <sys/types.h>

typedef struct{
    char **args;
}
Comando;

Comando *parsear_linea(char *linea, int *total_cmds);

//Ejecuta una tubería de N comandos.
//background == 0: espera a todos los hijos (foreground)
//background == 1: no espera (background)
//Guarda los PIDs de los hijos en pids_salida
//Devuelve el número de PIDs guardados, en caso de error -1
int ejecutar_tuberias(Comando *pipeline, int total_cmds, int background, pid_t *pids_salida, int max_pids);

void liberar_pipeline(Comando *pipeline, int total_cmds);

#endif
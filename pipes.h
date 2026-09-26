#ifndef PIPES_H
#define PIPES_H

typedef struct{
    char **args;
}
Comando;

Comando *parsear_linea(char *linea, int *total_cmds);
void ejecutar_tuberias(Comando *pipeline, int total_cmds);
void liberar_pipeline(Comando *pipeline, int total_cmds);

#endif
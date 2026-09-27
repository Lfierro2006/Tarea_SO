#ifndef REDIRECCION_H
#define REDIRECCION_H

//
int redireccion_parsear(char **argv, char **archivo_in, char **archivo_out, int *modo_append);
//
int redireccion_aplicar(const char *archivo_in, const char *archivo_out, int modo_append);

#endif
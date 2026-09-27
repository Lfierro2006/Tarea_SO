#include "redireccion.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "fcntl.h"


int redireccion_parsear(char **argv, char **archivo_in, char **archivo_out, int *modo_append) {
    *archivo_in = NULL;
    *archivo_out = NULL;
    *modo_append = 0;
    int i = 0;
    int j = 0;

    while (argv[i] != NULL) {
        //
        if (strcmp(argv[i], "<") == 0) {
            if (argv[i+1] == NULL) {
                fprintf(stderr, "Error Sintaxis, no existe archivo después de '<'\n");
                return -1;
            }
            *archivo_in = argv[i+1];
            i += 2;
        }
        //
        else if (strcmp(argv[i], ">") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Error sintaxis, esperaba un archivo despues de '>'\n");
                return -1;
            }
            *archivo_out = argv[i + 1];
            *modo_append = 0;
            i += 2;
        }
        // Redireccion de salida (agregar / append)
        else if (strcmp(argv[i], ">>") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Error sintaxis, se esperaba un archivo después de '>>'\n");
                return -1;
            }
            *archivo_out = argv[i + 1];
            *modo_append = 1;
            i += 2;
        }
        // Argumento normal del comando
        else {
            argv[j++] = argv[i++];
        }
    }
    // limpieza argumentos para execvp
    argv[j] = NULL; 
    return 0;
}

int redireccion_aplicar(const char *archivo_in, const char *archivo_out, int modo_append) {
    // Aplicar redireccion de entrada si existe
    if (archivo_in != NULL) {
        int fd_in = open(archivo_in, O_RDONLY);
        if (fd_in < 0) {
            perror(archivo_in);
            return -1;
        }
        if (dup2(fd_in, STDIN_FILENO) < 0) {
            perror("dup2 STDIN");
            close(fd_in);
            return -1;
        }
        close(fd_in); // Cierra fd original, la entrada ya quedo duplicada en STDIN
    }

    // Aplicar redireccion de salida si existe
    if (archivo_out != NULL) {
        int flags = O_WRONLY | O_CREAT | (modo_append ? O_APPEND : O_TRUNC);
        mode_t permisos = 0644; // rw-r--r--
        int fd_out = open(archivo_out, flags, permisos);
        if (fd_out < 0) {
            perror(archivo_out);
            return -1;
        }
        if (dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("dup2 STDOUT");
            close(fd_out);
            return -1;
        }
        close(fd_out); // Cierra fd original, la salida ya quedo en STDOUT
    }

    return 0;
}

/*
r3 checklist temporal

Reconocimiento al final o dentro de la línea de comandos (ciclo de while (argv[i] != NULL) recorre todo)
cmd > archivo   | done  | funciona  | crea o trunca
cmd >> archivo  | done  | funciona  | append
cmd < archivo   | done  | funciona  | stdin

Combinaciones válidas (sort < entrada.txt > salida.txt)
archivo_in y archivo_out son independientes, esto permite q se guarden en punteros independientes dentro de redireccion_parsear y se evalúan por separado en redireccion_aplicar

Llamadas al sistema exigidas (open(), dup2() y close() en el hijo)
tanto shellso como pipes, redireccion_aplcicar se invoca dentro del bloque de proceso hijo(pid == 0) con open(), luego dup2() hacia stdin_fileno o sdtout_fileno y cerrando con close()
*/
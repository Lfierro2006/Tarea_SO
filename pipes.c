#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "signal.h"
#include "pipes.h"
#include "redireccion.h"

Comando *parsear_linea(char *linea, int *total_cmds){ //usando strtok_r separa el string por cada "|" que encuentra
    int num_cmds =0;
    Comando *pipeline= NULL;
    char *saveptr_pipe;
    char *cmd_str= strtok_r(linea, "|", &saveptr_pipe);

    while (cmd_str!= NULL) {
        pipeline= realloc(pipeline, (num_cmds + 1) * sizeof(Comando));
        int num_args= 0;
        pipeline[num_cmds].args= NULL;

        // logica reemplazo de strtok: corta los argumentos y maneja las comillas que no podia el strtok
        char *lectura = cmd_str;
        char *escritura = cmd_str;

        while (*lectura != '\0') {
            // ignorar los espacios en blanco sobrantes antes de la palabra
            while (*lectura == ' ' || *lectura == '\t' || *lectura == '\n') {
                lectura++;
            }
            if (*lectura == '\0') {
                break;
            }

            // agregar el inicio de esta palabra al arreglo de argumentos
            pipeline[num_cmds].args = realloc(pipeline[num_cmds].args, (num_args + 1) * sizeof(char *));
            pipeline[num_cmds].args[num_args++] = escritura;

            // leer letra por letra para ver si estamos dentro de comillas
            int en_comillas = 0;
            char tipo_comilla = 0;

            while (*lectura != '\0') {
                if (!en_comillas && (*lectura == '"' || *lectura == '\'')) {
                    en_comillas = 1;
                    tipo_comilla = *lectura;
                    lectura++; 
                } else if (en_comillas && *lectura == tipo_comilla) {
                    en_comillas = 0;
                    lectura++; 
                } else if (!en_comillas && (*lectura == ' ' || *lectura == '\t' || *lectura == '\n')) {
                    lectura++; 
                    break;     
                } else {
                    *escritura = *lectura; 
                    escritura++;
                    lectura++;
                }
            }
            *escritura = '\0'; 
            escritura++;
        }

        //el ultimo indice del arreglo de comandos se coloca NULL para el requisito del execvp()
        pipeline[num_cmds].args= realloc(pipeline[num_cmds].args, (num_args + 1) * sizeof(char *));
        pipeline[num_cmds].args[num_args]= NULL;
        
        num_cmds++;
        cmd_str= strtok_r(NULL, "|", &saveptr_pipe);
    }

    *total_cmds= num_cmds;
    return pipeline;
}


int ejecutar_tuberias(Comando *pipeline, int total_cmds, int background, pid_t *pids_salida, int max_pids, char *linea_original, char *copia_linea) {
    int fd_in= 0;    //puente: lectura del pipe anerior
    int fd[2];       //tunel: fd[0] será lectura y fd[1] escritura respectiva
    int n_pids = 0;  // cuántos PIDs hemos guardado

    for (int i = 0; i < total_cmds; i++){
        //si no es el ultimo cmd de la iteracion se crea un pipe
        if (i < total_cmds - 1) {
            if(pipe(fd) < 0) {
                perror("Error en pipe");
                return -1;
            }
        }

        pid_t pid=fork();

        //PROCESO HIJO
        if (pid==0){    //si hay una conexion pendiente, une la entrada del pipe anterior
            // process background no deben verse afectados por Ctrl+C
            if (background) {
                setpgid(0, 0);
            }

            // restaurar disposicion por defecto de SIGINT y SIGQUIT en el hijo
            struct sigaction sa_dfl;
            sa_dfl.sa_handler = SIG_DFL;
            sigemptyset(&sa_dfl.sa_mask);
            sa_dfl.sa_flags = 0;
            sigaction(SIGINT, &sa_dfl, NULL);
            sigaction(SIGQUIT, &sa_dfl, NULL);

            if (fd_in!= 0){
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            if(i < (total_cmds - 1)){   //si no es el ultimo cmd, une la salida estandar al pipe actual y cierra lo q no usa
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }

            // R3: Redireccion de entrada/salida
            char *archivo_in = NULL;
            char *archivo_out = NULL;
            int modo_append = 0;

            if (redireccion_parsear(pipeline[i].args, &archivo_in, &archivo_out, &modo_append) < 0) {
                liberar_pipeline(pipeline,total_cmds);
                free(linea_original);
                free(copia_linea);
                _exit(EXIT_FAILURE);
            }

            if (redireccion_aplicar(archivo_in, archivo_out, modo_append) < 0) {
                liberar_pipeline(pipeline,total_cmds);
                free(linea_original);
                free(copia_linea);
                _exit(EXIT_FAILURE);
            }

            if (pipeline[i].args[0] == NULL) {
                liberar_pipeline(pipeline,total_cmds);
                free(linea_original);
                free(copia_linea);
                _exit(EXIT_FAILURE);
            }

            //ejecucion del comando
            execvp(pipeline[i].args[0], pipeline[i].args);
            perror("Error en execvp");

            liberar_pipeline(pipeline,total_cmds);
            free(linea_original);
            free(copia_linea);

            _exit(EXIT_FAILURE);
        }
        else if (pid < 0){
            perror("Error en fork");
            return -1;
        }

        //PROCESO PADRE
        
        // Guarda el PID del hijo
        if (n_pids < max_pids) {
            pids_salida[n_pids++] = pid;
        }
        if (fd_in!=0){  //cierra el fd actual sin usar
            close(fd_in);
        }
        if (i < (total_cmds - 1)){  //cierra el fd actual que no usa y guarda el de lectura para que el hijo lo conecte
            close(fd[1]);
            fd_in= fd[0];
        }
    }

    //Cierra el último fd_in si quedó abierto (recomendación externa)
    if (fd_in != 0) {
        close(fd_in);
    }    

    //Si es foreground, espera a todos los hijos del pipeline
    if (!background) {
        for (int i = 0; i < n_pids; i++) {
            waitpid(pids_salida[i], NULL, 0);
        }
    }
    return n_pids;
}


//funcion propuesta por gemini para no tener problemas con fugas de memoria (no entendi como se usaba pero la dejo aqui xsiacaso)
void liberar_pipeline(Comando *pipeline, int total_cmds) {
    for (int i = 0; i < total_cmds; i++) {
        free(pipeline[i].args);
    }
    free(pipeline);
}
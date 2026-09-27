#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "jobs.h"
#include "pipes.h"

//recolecta todos los hijos terminados
//sin bloquearse. Se llama de forma asíncrona.
static void manejador_sigchld(int sig) {
    (void)sig;
    int status;
    pid_t pid;

    // Bucle con WNOHANG (pregunta 3 del enunciado)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        jobs_marcar_terminado(pid);
    }
}

//Instala el manejador de SIGCHLD usando sigaction.
static void instalar_sigchld(void) {
    struct sigaction sa;
    sa.sa_handler = manejador_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}


int main(void) {
    // Punteros para la lectura dinámica de la línea
    char *linea = NULL;
    size_t capacidad = 0; 
    char cwd[1024];

    //SOLO TIENE FINES ESTÉTICOS, NADA MÁS
    printf("\033[2J\033[H");
    fflush(stdout);

    //inicializa la tabla
    jobs_init();

    //Instala el manejador de SIGCHLD
    instalar_sigchld();
    
    // Ciclo basico por ahora
    while (1){
        
        // Avisa los jobs terminados antes de mostrar el prompt
        jobs_avisar_terminados();

        // Muestra el prompt de nuestra shell con la direccion donde estamos trabajando
        if (getcwd(cwd,sizeof(cwd)) != NULL){
            printf("\033[1;3;44;96mShell-A:\033[0m\033[36m%s\033[0m$",cwd);
        } else {
            perror("getcwd error");
        }
        
        // Fuerza el vaciado del buffer inmediatamente en la terminal
        fflush(stdout);
        
        // Se lee linea completa con getline
        if (getline(&linea, &capacidad, stdin) == -1) {
            printf("\n");
            free(linea); // Liberamos la memoria de la línea al salir con Ctrl+D
            break;
        }

        // Detecta si hay un & al final
        int background = 0;
        size_t len = strlen(linea);
        // Como len-1 corresponde al ultimo caracter, "retrocede" cuando hay '\n', ' ' y '\t'
        while (len > 0 && (linea[len-1] == '\n' || linea[len-1] == ' ' || linea[len-1] == '\t')) {
            len--;
        }
        if (len > 0 && linea[len-1] == '&') {
            background = 1;
            linea[len-1] = '\0';   // quita el &
        }

        //Guarda una copia de linea en cmdline_para_jobs del comando para jobs y Done,
        //esto porque más adelante linea se tokeniza
        char cmdline_para_jobs[1024];
        strncpy(cmdline_para_jobs, linea, sizeof(cmdline_para_jobs) - 1);
        cmdline_para_jobs[sizeof(cmdline_para_jobs) - 1] = '\0';
        cmdline_para_jobs[strcspn(cmdline_para_jobs, "\n")] = '\0';

        //Si la línea tiene pipes
        if (strchr(linea, '|') != NULL){
            //Duplica la línea porque parsear_linea la modifica con strtok_r
            char *copia = malloc(strlen(linea) + 1);
            strcpy(copia, linea);
            int total_cmds = 0;
            Comando *pipeline = parsear_linea(copia, &total_cmds);

            if (pipeline != NULL && total_cmds > 0) {
                pid_t pids[16];
                int n = ejecutar_tuberias(pipeline, total_cmds, background, pids, 16);
                if (background && n > 0){
                    int job_id = jobs_agregar(pids, n, cmdline_para_jobs);
                    if (job_id > 0){
                        printf("[%d] %d\n", job_id, pids[0]);
                    }
                }
                liberar_pipeline(pipeline, total_cmds);
            }
            free(copia);
            continue;   // vuelve al inicio del ciclo
        }

        // Variables dinámicas para los argumentos en cada iteración
        int argc = 0;
        char **argv = NULL; 
        char *token = strtok(linea, " \t\n");

        // Mientras queden palabras, agrandamos la memoria un bloque a la vez
        while (token != NULL) {
            argv = realloc(argv, (argc + 1) * sizeof(char *));
            argv[argc++] = token;
            token = strtok(NULL, " \t\n");
        }
        
        // Si solo le damos a enter en la shell, liberamos y volvemos al inicio
        if (argc == 0){
            free(argv);
            continue;
        }

        // Agregamos un bloque extra para el NULL obligatorio de execvp
        argv = realloc(argv, (argc + 1) * sizeof(char *));
        argv[argc] = NULL;

        // Para probar nomas, ponganlo como comentario si es muy incomodo
        
        /* printf("Comando: [%s]\n", argv[0]);
        for(int i = 1; i < argc; i++){
            printf(" Argumento [%d]: %s\n", i , argv[i]);
        } */


        // Comandos Built In:

        // Exit
        if (strcmp(argv[0], "exit") == 0){
            int exit_status = 0;
            if (argc > 1){
                exit_status = atoi(argv[1]);
            }
            // Limpiamos la memoria antes de destruir el proceso
            free(linea); 
            free(argv);
            exit(exit_status);
        }

        // cd
        if (strcmp(argv[0], "cd") == 0){
            const char *dir = (argc > 1)? argv[1] : getenv("HOME");
            if (dir == NULL || chdir(dir) < 0){
                perror("cd error");
            }
            free(argv);
            continue;
        }

        // Jobs
        if(strcmp(argv[0], "jobs")== 0){
            jobs_listar();
            free(argv);
            continue;
        }
        
        // Pmon
        if(strcmp(argv[0], "pmon")==0){
            printf("Falta por hacer pmon\n");
            free(argv);
            continue;
        }

        // Partir el fork
        pid_t pid = fork();
        if (pid < 0){
            perror("Error de fork");
            free(argv);
            free(linea);
            break;
        }
        // Hijo
        else if(pid == 0){
            execvp(argv[0], argv);
            perror("Comando no existe");
            free(argv);
            free(linea);
            _exit(127);
        }
        
        // Padre
        else{
            if (background) {
                //registra el job y vuelve al prompt
                pid_t pids[1];
                pids[0] = pid;

                int job_id = jobs_agregar(pids, 1, cmdline_para_jobs);
                if (job_id > 0) {
                    printf("[%d] %d\n", job_id, pid);
                }
            } else {
                //espera normal
                int status;
                if (waitpid(pid, &status, 0) < 0){
                    perror("Error en waitpid");
                }
            }
        }
        
        // Limpiamos el arreglo de argumentos actual antes de pedir la siguiente línea
        free(argv);
    }
    
    return 0;
}
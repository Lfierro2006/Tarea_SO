#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "jobs.h"
#include "pipes.h"
#include "redireccion.h"

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

    // cambio R6 shell ignora SIGINT y SIGQUIT
    struct sigaction sa_ign;
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    sa_ign.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_ign, NULL);
    sigaction(SIGQUIT, &sa_ign, NULL);
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
            printf("\033[1;3;44;96mmishell:\033[0m\033[36m%s\033[0m$",cwd);
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
            //Duplica la línea porque parsear_linea la modifica internamente
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
        // logica reemplazo de strtok: corta los argumentos y maneja las comillas que no podia el strtok
        char *lectura = linea;
        char *escritura = linea;

        while (*lectura != '\0') {
            // ignorar los espacios en blanco sobrantes antes de la palabra
            while (*lectura == ' ' || *lectura == '\t' || *lectura == '\n') {
                lectura++;
            }
            if (*lectura == '\0') {
                break;
            }

            // agregar el inicio de esta palabra al arreglo de argumentos
            argv = realloc(argv, (argc + 1) * sizeof(char *));
            argv[argc++] = escritura;

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
        
        // Si solo le damos a enter en la shell, liberamos y volvemos al inicio
        if (argc == 0){
            free(argv);
            continue;
        }

        // Agregamos un bloque extra para el NULL obligatorio de execvp
        argv = realloc(argv, (argc + 1) * sizeof(char *));
        argv[argc] = NULL;

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
        if (strcmp(argv[0], "pmon") == 0) {
            int seg = 2; // Valor por defecto si se omitieran segundos
            if (argc > 1) {
                seg = atoi(argv[1]);
                if (seg <= 0) {
                    seg = 2;
                }
            }
            jobs_pmon(seg);
            free(argv);
            continue;
        }

        // r3, Parsear redirecciones antes del fork
        char *archivo_in = NULL;
        char *archivo_out = NULL;
        int modo_append = 0;

        if (redireccion_parsear(argv, &archivo_in, &archivo_out, &modo_append) < 0) {
            free(argv);
            continue;
        }

        if (argv[0] == NULL) {
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
            // Aislar procesos en background de Ctrl+C
            if (background) {
                setpgid(0, 0);
            }

            // Restaurar disposicion por defecto en el hijo
            struct sigaction sa_dfl;
            sa_dfl.sa_handler = SIG_DFL;
            sigemptyset(&sa_dfl.sa_mask);
            sa_dfl.sa_flags = 0;
            sigaction(SIGINT, &sa_dfl, NULL);
            sigaction(SIGQUIT, &sa_dfl, NULL);

            // Aplicar redirecciones en el hijo
            if (redireccion_aplicar(archivo_in, archivo_out, modo_append) < 0) {
                free(argv);
                free(linea);
                _exit(EXIT_FAILURE);
            }

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


/* checklist r6

La shell principal ignore SIGINT y SIGQUIT | hecho
en instalar_sigchld() de shellso.c, se configura sa_ign.sa_handler = SIG_IGN tanto para SIGINT como para SIGQUIT

Los procesos hijos (pid == 0) restauren el comportamiento por defecto (SIG_DFL) antes de ejecutar execvp

Terminar únicamente el comando en primer plano con Ctrl+C
Tanto en shellso.c  como en pipes.c , dentro del bloque if (pid == 0) —exactamente después del fork() y antes del execvp(

Proteger los procesos en background frente a Ctrl+C
proceso hijo (pid == 0) de shellso.c y pipes.c, se evalúa if (background) { setpgid(0, 0); }

Uso estricto de sigaction() con sa_mask y sa_flags
En todo el proyecto se usa exclusivamente sigaction(), inicializando las máscaras con sigemptyset y aplicando el flag SA_RESTART para que llamadas bloqueantes como getline() no llegasuen a fallar

*/
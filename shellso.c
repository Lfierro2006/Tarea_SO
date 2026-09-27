#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(void) {
    // Punteros para la lectura dinámica de la línea
    char *linea = NULL;
    size_t capacidad = 0; 
    char cwd[1024];

    // Ciclo basico por ahora
    while (1){
        // Muestra el prompt de nuestra shell con la direccion donde estamos trabajando
        if (getcwd(cwd,sizeof(cwd)) != NULL){
            printf("Shell-A:%s$ ", cwd);
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
            printf("Falta por hacer el job\n");
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
            int status;
            if (waitpid(pid, &status,0) < 0){
                perror("Error en waitpid");
            }
        }
        
        // Limpiamos el arreglo de argumentos actual antes de pedir la siguiente línea
        free(argv);
    }
    
    return 0;
}
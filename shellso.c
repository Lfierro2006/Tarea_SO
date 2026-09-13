# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/wait.h>

int main(void) {
    char linea[1024];
    char cwd[1024];
    char *argv[64];

    //Ciclo basico por ahora
    while (1){
        //Muestra el prompt de nuestra shell con la direccion donde estamos trabajando
        if (getcwd(cwd,sizeof(cwd)) != NULL){
            printf("Shell-A:%s$ ", cwd);
        } else {
            perror("getcwd error");
        }
        // Fuerza el vaciado del buffer inmediatamente en la terminal
        fflush(stdout);
        // Se lee linea completa
        if (fgets(linea, sizeof(linea),stdin ) == NULL) {
            //Si la linea leida es nula, imprime un salto de linea y termina el codigo
            printf("\n");
            break;
        }

        int argc = 0;
        char *token = strtok(linea, " \t\n");

        //Mientras queden por cortar y no pasemos del limite, seguimos cortando
        while ((token != NULL) && (argc < 63))
        {
            argv[argc++] = token;
            token = strtok(NULL," \t\n");
        }
        argv[argc] = NULL;

        //Si solo le damos a enter en la shell, nos manda devuelta
        if (argc == 0){
            continue;
        }

        //Para probar nomas, ponganlo como comentario si es muy incomodo
        printf("Comando: [%s]\n", argv[0]);
        for(int i = 1; i < argc; i++){
            printf(" Argumento [%d]: %s\n", i , argv[i]);
        }

        //Comandos Built In:

        //Exit
        if (strcmp(argv[0], "exit") == 0){
            //el estatus default
            int exit_status = 0;
            //estatus especifico
            if (argc > 1){
                exit_status = atoi(argv[1]);
            }
            exit(exit_status);
        }

        //cd
        if (strcmp(argv[0], "cd") == 0){
            //Si tiene argumento, se usa, sino, se usa Home
            const char *dir = (argc > 1)? argv[1] : getenv("HOME");
            //chdir() cambia el dir del proceso
            if (dir == NULL || chdir(dir) < 0){
                perror("cd error");
            }
            continue;
        }
        //Jobs
        if(strcmp(argv[0], "jobs")== 0){
            printf("Falta por hacer el job\n");
            continue;
        }
        //Pmon
        if(strcmp(argv[0], "pmon")==0){
            printf("Falta por hacer pmon\n");
            continue;
        }

        //Partir el fork
        pid_t pid = fork();
        if (pid < 0){
            perror("Error de fork");
            break;
        }
        //Hijo
        else if(pid == 0){
            //Reemplaza el hijo con lo que se pide a la shell (el argv[0])
            execvp(argv[0], argv);
            //Solo se ejecuta lo de abajo si falla el execvp
            perror("Comando no existe");
            exit(127);
        }

        //Padre
        else{
            int status;
            //Esperando a que hijo termine de ejecutar antes de ciclar de vuelta
            if (waitpid(pid, &status,0) < 0){
                perror("Error en waitpid");
            }
        }
    }
    return 0;
}
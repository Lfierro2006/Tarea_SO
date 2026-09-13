# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>

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

        //Para probar nomas
        printf("Comando: [%s]\n", argv[0]);
        for(int i = 1; i < argc; i++){
            printf(" Argumento: %d [%s]\n", i , argv[i]);
        }
    }
    return 0;
}
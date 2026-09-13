# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>

int main(void) {
    char linea[1024];
    char cwd[1024];

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
    }
    return 0;
}
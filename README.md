# Tarea_SO
Una implementación de un intérprete de comandos (shell) en C para entornos POSIX/Linux. Este proyecto soporta la ejecución de comandos en primer y segundo plano, tuberías de largo arbitrario, redirección de entrada/salida, manejo robusto de señales y un monitor de procesos propio (`pmon`).

**Integrantes:**

| Integrante | Matricula |
| :--- | :--- |
| Matias Cabello | 2025404117 |
| Lucas Fierro | 2025423669  |
| Eduardo Riveros  | 2025432692  |
| Vicente Vergara | 2025431734  |

## Instrucciones de Compilacion:

El proyecto incluye un `Makefile` configurado para compilar todos los módulos automáticamente con las banderas de advertencia y depuración necesarias (`-Wall -Wextra -std=gnu11 -g`).

- Abre una terminal y posiciónate en el directorio raíz del proyecto.
- Ejecuta "make" en la terminal para compilar el codigo de la shell y sacar el ejecutable
- Una vez que compile, ejecuta la shell usando "./mishell" en la terminal
- De ahi se vera la shell con su prompt personalizado para poder ejecutar comandos en ella. 

## Instrucciones de Ejecución:

- Dentro de la shell uno tiene acceso a comandos simples mediante escritura directa en la shell
- Para poder trabajar comandos multiples dentro de la shell, uno debe escribir la serie de comandos acompañado de el simbolo "|" entre cada comando que siga al inicial.
- Si uno desea ejecutar un comando en segundo plano, debe usar el sufijo "&" al final del comando que desea trabajar en segundo plano.
- Si uno quiere terminar el uso de la shell, escribir exit n (opcional) o presionar (CTRL+D) (EOF) al ubicarse dentro de ella.


## Funcionalidades soportadas:
- Ejecución de comandos externos: Soporte nativo a través de PATH usando execvp.
- Tuberías (Pipes): Encadenamiento de múltiples comandos de largo arbitrario (ej. ls -l | grep ".c" | wc -l).
- Redireccion de E/S:
    - "<" archivo: Redirige la entrada estándar.
    - ">" archivo: Redirige y trunca la salida estándar.
    - ">>" archivo: Redirige y adjunta (append) a la salida estándar.
- Background: Agrega "&" al final de un comando para ejecutarlo en segundo plano. La shell notificará asíncronamente cuando el proceso termine mediante un manejador de SIGCHLD.
- Manejo de Señales: La shell es inmune a Ctrl+C (SIGINT) y Ctrl+\ (SIGQUIT). Presionar Ctrl+C solo interrumpirá el comando que se esté ejecutando en primer plano en ese momento.

## Comandos Internos:
- cd [dir]: Cambia el directorio actual a dir. Si no dan argumentos se va a $HOME
- exit [n]: Termina la shell con codigo de salida n.
- jobs: Lista los procesos en segundo plano activas y detenidos.
- pmon [segundos]: Inicia el monitor de procesos en tiempo real que muestra el estado, % uso de CPU, consumo de la memoria (RSS) de los procesos 
en background. Uno puede salir de pmon con CTRL+C.

## Funcionalidades adicionales:
- Control de Trabajos (Ctrl+Z): Presionar Ctrl+Z suspende el proceso actual en foreground y lo envía a la tabla de trabajos en estado Detenido.
- fg [id]: Trae un trabajo detenido o en background hacia el primer plano (foreground).
- bg [id]: Reanuda un trabajo detenido para que continúe su ejecución asíncrona en segundo plano.
- Orden de tabla de pmon por %CPU (similar a top del bash de Linux) y resaltar en rojo el proceso de mas uso.

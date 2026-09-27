#include "jobs.h"
#include <string.h>
#include <stdio.h>

//Tabla global de jobs
static Job tabla_jobs[MAX_JOBS];
//Contador para asignar ids
static int siguiente_id = 1;

//Limpia la tabla al iniciar la shell
void jobs_init(void) {
    memset(tabla_jobs, 0, sizeof(tabla_jobs));
    siguiente_id = 1;
}

//Agrega un job nuevo a la tabla
//Retorna su id (debe ser >=1) o -1 si no había espacio
int jobs_agregar(pid_t *pids, int n_pids, const char *cmdline) {
    
    //Busca un espacio libre en la tabla
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!tabla_jobs[i].activo) {
            tabla_jobs[i].id = siguiente_id++;

            //Copia los PID, si se excede guarda el máximo posible
            tabla_jobs[i].n_pids = (n_pids > MAX_PIDS_POR_JOB)? MAX_PIDS_POR_JOB : n_pids;
            for (int j = 0; j < tabla_jobs[i].n_pids; j++) {
                tabla_jobs[i].pids[j] = pids[j];
            }

            //Copia el comando, el '\0' se agrega por si acaso
            strncpy(tabla_jobs[i].cmdline, cmdline, sizeof(tabla_jobs[i].cmdline) - 1);
            tabla_jobs[i].cmdline[sizeof(tabla_jobs[i].cmdline) - 1] = '\0';

            //Estado inicial
            tabla_jobs[i].estado = JOB_EJECUTANDO;
            tabla_jobs[i].activo = 1;
            tabla_jobs[i].avisado = 0;

            return tabla_jobs[i].id;
        }
    }
    return -1;
}

//Muestra los jobs
void jobs_listar(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!tabla_jobs[i].activo) continue;

        //Texto del estado según el enum
        const char *estado_str;
        if (tabla_jobs[i].estado == JOB_EJECUTANDO) {
            estado_str = "Ejecutando";
        }
        else if (tabla_jobs[i].estado == JOB_TERMINADO) {
            estado_str = "Terminado";
        }else{
            estado_str = "Detenido";
        }

        printf("[%d] %-12s %s\n",tabla_jobs[i].id, estado_str, tabla_jobs[i].cmdline);
    }
}

//Marca como terminado el job del PID dado
void jobs_marcar_terminado(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!tabla_jobs[i].activo) continue;

        for (int j = 0; j < tabla_jobs[i].n_pids; j++) {
            if (tabla_jobs[i].pids[j] == pid) {
                tabla_jobs[i].estado = JOB_TERMINADO;
                return;
            }
        }
    }
}

//Recorre la tabla y muestra los jobs terminados que aun
//no avisan que terminaron
//Libera el slot para reutilizarlo.
void jobs_avisar_terminados(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!tabla_jobs[i].activo) continue;

        if (tabla_jobs[i].estado == JOB_TERMINADO && !tabla_jobs[i].avisado) {

            //se le agrega un salto de linea por densidad de contenido, aunque basta con el color
            printf("\n\033[32m[%d]+ Done\033[0m %s\n",tabla_jobs[i].id,tabla_jobs[i].cmdline);
            tabla_jobs[i].avisado = 1;
            tabla_jobs[i].activo  = 0;
        }
    }
}

// Función auxiliar para buscar un job por ID RESERVADO BONUS CTRL+Z
/* Job *jobs_buscar_por_id(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (tabla_jobs[i].activo && tabla_jobs[i].id == job_id) {
            return &tabla_jobs[i];
        }
    }
    return NULL;
}
 */
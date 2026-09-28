#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

// Máximo de jobs simultáneos en background
#define MAX_JOBS 64
// Máximo de procesos por job (para pipelines)
#define MAX_PIDS_POR_JOB 16

// Estado posible de un job
typedef enum {
    JOB_EJECUTANDO=0,
    JOB_TERMINADO,
    JOB_DETENIDO     // reservado para el bonus de Ctrl+Z
} JobEstado;

// Un job
typedef struct {
    int        id;                       //número
    pid_t      pids[MAX_PIDS_POR_JOB];   //PIDs de todos los procesos del job
    pid_t      pgid;                     //Id del grupo de procesos del job PARA BONUS CTRL+Z
    int        n_pids;                   //cuántos PIDs tiene
    char       cmdline[1024];            //texto del comando para mostrar
    JobEstado  estado;                   //ejecutando/terminado/detenido
    int        activo;                   //1=espacio ocupado, 0=espacio libre
    int        avisado;                  //1 significa que ya se mostró "Done"
} Job;

// Inicializa la tabla de jobs. Debe llamarse una vez al inicio
void jobs_init(void);

//Agrega un job nuevo a la tabla
//Retorna su id (debe ser >=1) o -1 si no había espacio
int jobs_agregar(pid_t *pids, int n_pids, const char *cmdline);

//Muestra los jobs
void jobs_listar(void);

// Marca como terminado el job del PID dado
void jobs_marcar_terminado(pid_t pid);

//Recorre la tabla y muestra "[id]+ Done cmd" para los jobs
//que terminaron y aún no fueron avisados y libera el slot.
//Se llama desde el ciclo principal, no desde el manejador.
void jobs_avisar_terminados(void);

Job *jobs_buscar_por_id(int job_id);

void jobs_marcar_detenido(pid_t pid);
int jobs_fg(int job_id);
int jobs_bg(int job_id);

//
void jobs_pmon(int segundos);

#endif
#include "jobs.h"
#include <signal.h>
<<<<<<< HEAD
#include <stdio.h>
=======
>>>>>>> 8bcc01e98989ceccc5707bec59089c5f61f59cbf
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


// Tabla global de jobs
static Job tabla_jobs[MAX_JOBS];
// Contador para asignar ids
static int siguiente_id = 1;

// Limpia la tabla al iniciar la shell
void jobs_init(void) {
  memset(tabla_jobs, 0, sizeof(tabla_jobs));
  siguiente_id = 1;
}

// Agrega un job nuevo a la tabla
// Retorna su id (debe ser >=1) o -1 si no había espacio
int jobs_agregar(pid_t *pids, int n_pids, const char *cmdline) {

  // Busca un espacio libre en la tabla
  for (int i = 0; i < MAX_JOBS; i++) {
    if (!tabla_jobs[i].activo) {
      tabla_jobs[i].id = siguiente_id++;

      // Copia los PID, si se excede guarda el máximo posible
      tabla_jobs[i].n_pids =
          (n_pids > MAX_PIDS_POR_JOB) ? MAX_PIDS_POR_JOB : n_pids;
      for (int j = 0; j < tabla_jobs[i].n_pids; j++) {
        tabla_jobs[i].pids[j] = pids[j];
      }

      // Copia el comando, el '\0' se agrega por si acaso
      strncpy(tabla_jobs[i].cmdline, cmdline,
              sizeof(tabla_jobs[i].cmdline) - 1);
      tabla_jobs[i].cmdline[sizeof(tabla_jobs[i].cmdline) - 1] = '\0';

      // Estado inicial
      tabla_jobs[i].estado = JOB_EJECUTANDO;
      tabla_jobs[i].activo = 1;
      tabla_jobs[i].avisado = 0;

      return tabla_jobs[i].id;
    }
  }
  return -1;
}

// Muestra los jobs
void jobs_listar(void) {
  for (int i = 0; i < MAX_JOBS; i++) {
    if (!tabla_jobs[i].activo)
      continue;

    // Texto del estado según el enum
    const char *estado_str;
    if (tabla_jobs[i].estado == JOB_EJECUTANDO) {
      estado_str = "Ejecutando";
    } else if (tabla_jobs[i].estado == JOB_TERMINADO) {
      estado_str = "Terminado";
    } else {
      estado_str = "Detenido";
    }

    printf("[%d] %-12s %s\n", tabla_jobs[i].id, estado_str,
           tabla_jobs[i].cmdline);
  }
}

// Marca como terminado el job del PID dado
void jobs_marcar_terminado(pid_t pid) {
  for (int i = 0; i < MAX_JOBS; i++) {
    if (!tabla_jobs[i].activo)
      continue;

    for (int j = 0; j < tabla_jobs[i].n_pids; j++) {
      if (tabla_jobs[i].pids[j] == pid) {
        tabla_jobs[i].estado = JOB_TERMINADO;
        return;
      }
    }
  }
}

// Marca como detenido el job del PID dado
void jobs_marcar_detenido(pid_t pid) {
  for (int i = 0; i < MAX_JOBS; i++) {
    if (!tabla_jobs[i].activo)
      continue;

    for (int j = 0; j < tabla_jobs[i].n_pids; j++) {
      if (tabla_jobs[i].pids[j] == pid) {
        tabla_jobs[i].estado = JOB_DETENIDO;
        return;
      }
    }
  }
}

// Recorre la tabla y muestra los jobs terminados que aun
// no avisan que terminaron
// Libera el slot para reutilizarlo.
void jobs_avisar_terminados(void) {
  for (int i = 0; i < MAX_JOBS; i++) {
    if (!tabla_jobs[i].activo)
      continue;

    if (tabla_jobs[i].estado == JOB_TERMINADO && !tabla_jobs[i].avisado) {

      // se le agrega un salto de linea por densidad de contenido, aunque basta
      // con el color
      printf("\n\033[32m[%d]+ Done\033[0m %s\n", tabla_jobs[i].id,
             tabla_jobs[i].cmdline);
      tabla_jobs[i].avisado = 1;
      tabla_jobs[i].activo = 0;
    }
  }
}

// Función auxiliar para buscar un job por ID
Job *jobs_buscar_por_id(int job_id) {
  for (int i = 0; i < MAX_JOBS; i++) {
    if (tabla_jobs[i].activo && tabla_jobs[i].id == job_id) {
      return &tabla_jobs[i];
    }
  }
  return NULL;
}

int jobs_fg(int job_id) {
  Job *job = jobs_buscar_por_id(job_id);
  if (!job) {
    printf("fg: job %d no encontrado\n", job_id);
    return -1;
  }

  // Suponemos que pgid = pids[0] ya que setpgid(0, pids[0]) en mishell
  pid_t pgid = job->pids[0];

  // Damos control del terminal al grupo del job
  tcsetpgrp(STDIN_FILENO, pgid);

  // Marcamos ejecutando y enviamos SIGCONT
  job->estado = JOB_EJECUTANDO;
  kill(-pgid, SIGCONT);

  // Esperamos por los procesos del job (mismo comportamiento que !background)
  for (int i = 0; i < job->n_pids; i++) {
    int status;
    pid_t res = waitpid(job->pids[i], &status, WUNTRACED);
    if (res > 0) {
      if (WIFSTOPPED(status)) {
        job->estado = JOB_DETENIDO;
        printf("\n[%d]+ Detenido %s\n", job->id, job->cmdline);
      } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
        jobs_marcar_terminado(job->pids[i]);
      }
    }
  }

  // Devolvemos control a la shell
  tcsetpgrp(STDIN_FILENO, getpgrp());
  return 0;
}

int jobs_bg(int job_id) {
  Job *job = jobs_buscar_por_id(job_id);
  if (!job) {
    printf("bg: job %d no encontrado\n", job_id);
    return -1;
  }
  pid_t pgid = job->pids[0];
  job->estado = JOB_EJECUTANDO;
  printf("[%d]+ %s &\n", job->id, job->cmdline);
  kill(-pgid, SIGCONT);
  return 0;
}

static volatile sig_atomic_t pmon_flag_alarma = 0;
static volatile sig_atomic_t pmon_flag_salir = 0;

static void manejador_pmon_sigalrm(int sig) {
  (void)sig;
  pmon_flag_alarma = 1;
}

static void manejador_pmon_sigint(int sig) {
  (void)sig;
  pmon_flag_salir = 1;
}

// Lee proc/[pid] u stat para obtener state y utime+stime
static int leer_proc_stat(pid_t pid, char *estado_out,
                          unsigned long *ticks_out) {
  char ruta[64];
  snprintf(ruta, sizeof(ruta), "/proc/%d/stat", (int)pid);

  FILE *f = fopen(ruta, "r");
  if (f == NULL) {
    return -1;
  }

  char buffer[1024];
  if (fgets(buffer, sizeof(buffer), f) == NULL) {
    fclose(f);
    return -1;
  }
  fclose(f);

  char *fin_comm = strrchr(buffer, ')');
  if (fin_comm == NULL || *(fin_comm + 1) == '\0') {
    return -1;
  }

  char state = '?';
  unsigned long utime = 0;
  unsigned long stime = 0;

  // A partir de fin_comm + 2 empieza el campo 3
  int leidos =
      sscanf(fin_comm + 2, "%c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
             &state, &utime, &stime);
  if (leidos != 3) {
    return -1;
  }

  *estado_out = state;
  *ticks_out = utime + stime;
  return 0;
}

// Lee /proc/[pid] u status para obtener el campo VmRSS en KB
static long leer_proc_rss(pid_t pid) {
  char ruta[64];
  snprintf(ruta, sizeof(ruta), "/proc/%d/status", (int)pid);

  FILE *f = fopen(ruta, "r");
  if (f == NULL) {
    return 0;
  }

  char linea[256];
  long rss = 0;
  while (fgets(linea, sizeof(linea), f) != NULL) {
    if (strncmp(linea, "VmRSS:", 6) == 0) {
      sscanf(linea + 6, "%ld", &rss);
      break;
    }
  }
  fclose(f);
  return rss;
}

static const char *traducir_estado(char s) {
  if (s == 'R')
    return "ejecutando";
  if (s == 'S' || s == 'D')
    return "durmiendo";
  if (s == 'Z')
    return "zombie";
  if (s == 'T' || s == 't')
    return "detenido";
  return "desconocido";
}

// Estructura auxiliar para guardar la lectura previa de ticks de cada PID
typedef struct {
  pid_t pid;
  unsigned long ticks_prev;
  int valido;
} MuestraCPU;

void jobs_pmon(int segundos) {
  if (segundos <= 0) {
    segundos = 2;
  }

  pmon_flag_alarma =
      1; // En 1 para que dibuje la tabla inmediatamente al entrar
  pmon_flag_salir = 0;

  // Instalar manejadores de SIGALRM y SIGINT (sin SA_RESTART para despertar a
  // pause())
  struct sigaction sa_alrm, sa_old_alrm;
  struct sigaction sa_int, sa_old_int;

  sa_alrm.sa_handler = manejador_pmon_sigalrm;
  sigemptyset(&sa_alrm.sa_mask);
  sa_alrm.sa_flags = 0;
  sigaction(SIGALRM, &sa_alrm, &sa_old_alrm);

  sa_int.sa_handler = manejador_pmon_sigint;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = 0;
  sigaction(SIGINT, &sa_int, &sa_old_int);

  long clk_tck = sysconf(_SC_CLK_TCK);
  if (clk_tck <= 0) {
    clk_tck = 100;
  }

  MuestraCPU muestras[256];
  memset(muestras, 0, sizeof(muestras));

  while (!pmon_flag_salir) {
    if (pmon_flag_alarma) {
      pmon_flag_alarma = 0;

      // Limpieza y retiro de jobs que hayan terminado mientras pmon está activo
      jobs_avisar_terminados();

      // Limpiar pantalla y mostrar encabezado
      printf("\033[2J\033[H");
      printf("Monitor pmon (refresco: %d s) — Presione Ctrl+C para salir\n\n",
             segundos);
      printf("%-8s %-20s %-14s %-14s %-10s\n", "PID", "COMANDO", "ESTADO",
             "%CPU(aprox)", "RSS(KB)");
      printf("-----------------------------------------------------------------"
             "-----\n");

      MuestraCPU nuevas_muestras[256];
      int n_nuevas = 0;

      for (int i = 0; i < MAX_JOBS; i++) {
        if (tabla_jobs[i].activo) {
          for (int j = 0; j < tabla_jobs[i].n_pids; j++) {
            pid_t pid = tabla_jobs[i].pids[j];
            char state = '?';
            unsigned long ticks_actual = 0;

            if (leer_proc_stat(pid, &state, &ticks_actual) == 0) {
              long rss = leer_proc_rss(pid);
              double cpu_pct = 0.0;

              for (int k = 0; k < 256; k++) {
                if (muestras[k].valido && muestras[k].pid == pid) {
                  unsigned long delta_ticks = 0;
                  if (ticks_actual >= muestras[k].ticks_prev) {
                    delta_ticks = ticks_actual - muestras[k].ticks_prev;
                  }
                  double seg_cpu = (double)delta_ticks / (double)clk_tck;
                  cpu_pct = (seg_cpu / (double)segundos) * 100.0;
                  break;
                }
              }

              if (n_nuevas < 256) {
                nuevas_muestras[n_nuevas].pid = pid;
                nuevas_muestras[n_nuevas].ticks_prev = ticks_actual;
                nuevas_muestras[n_nuevas].valido = 1;
                n_nuevas++;
              }

              printf("%-8d %-20.20s %-14s %-14.1f %-10ld\n", (int)pid,
                     tabla_jobs[i].cmdline, traducir_estado(state), cpu_pct,
                     rss);
            }
          }
        }
      }

      // Actualizar muestras previas para el siguiente ciclo
      memset(muestras, 0, sizeof(muestras));
      for (int k = 0; k < n_nuevas; k++) {
        muestras[k] = nuevas_muestras[k];
      }

      fflush(stdout);
      alarm(segundos);
    }

    // Esperar señal (SIGALRM, SIGINT o SIGCHLD) sin consumir CPU
    pause();
  }
  alarm(0);
  sigaction(SIGALRM, &sa_old_alrm, NULL);
  sigaction(SIGINT, &sa_old_int, NULL);
  printf("\n");
}

/*  pmon objetivos

Leer /proc/[pid]/stat para extraer el estado (R, S, Z, T) y los tiempos de CPU
(utime y stime) sin usar

Leer /proc/[pid]/status buscando la línea VmRSS: para obtener la memoria
residente en KB

Calcular %CPU(aprox) comparando la diferencia (delta) de utime + stime entre dos
lecturas sucesivas dividida por los clock ticks del sistema
(sysconf(_SC_CLK_TCK)) y el intervalo de segundos

Controlar el temporizador y la salida por señales usando alarm(segundos) y
banderas volatile sig_atomic_t tanto para SIGALRM (refrescar tabla) como para
SIGINT (salir limpiamente con Ctrl+C sin cerrar la shell).

*/
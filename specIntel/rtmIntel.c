#include "rtmIntel.h"

//RIC Defino los global retries en rtmIntel.h como define para que no se introduzca en el Read Set de la transacción
//volatile char _gpad0[CACHE_BLOCK_SIZE];
//long global_retries = 3;

//RIC No podemos presumir que el compilador deje las variables locales tal y como se definen aquí
//    De hecho, he estado disassembling el código y gpad1, ticket y gpad2 estaban una detrás de otra,
//    sin embargo turn estaba en un sitio completamente diferente.
//    Esto puede hacer que haya falsos conflictos por false sharing si turn cae en una línea de caché con
//    otra variable que se escriba fuera de transacción.
//    Solución: meterlo en un struct. Lo meto en rtmIntel.h

/*volatile char _gpad1[CACHE_BLOCK_SIZE];
//RIC Para implementar el spinlock del fallback de Haswell
volatile unsigned int g_ticketlock_ticket = 0;
volatile unsigned int g_ticketlock_turn = 1;
volatile char _gpad2[CACHE_BLOCK_SIZE];*/

//RIC Archivo de estadísticas

unsigned long barrierCounter = 0;



char fname[256];
long threadCount;
long xactCount;
struct Stats **stats;
struct TicketLock g_ticketlock; //Inicializo en statsFileInit el ticketlock

volatile int sense = 0;
volatile int count = 0;

pthread_mutex_t bar_lock;
pthread_mutex_t global_lock;

fback_lock_t g_fallback_lock = {.ticket = 0, .turn = 1};
g_spec_vars_t g_specvars = {.tx_order = 1};

int statsFileInit(int argc, char **argv, long thCount, long xCount)
{
  int i, j;
  char ext[25];
  //Inicializo el ticket lock
  g_ticketlock.ticket = 0;
  g_ticketlock.turn = 1;

  //Saco la extensión con identificador de proceso para tener un archivo único
  sprintf(ext, "_%d.tmstats", getpid());
  //El nombre del archivo es la llamada entera al programa con sus parámetros
  strcat(fname, "./results/");
  strcat(fname, &(argv[0][2])); //Pongo el nombre del programa sin el ./
  for (i = 1; i < argc; i++)
  {
    strcat(fname, "_");
    if (strstr(argv[i], "./timeseries/"))
    {
      strcat(fname, &(argv[i][13]));
    }
    else
    {
      if (strstr(argv[i], "timeseries/"))
      {
        strcat(fname, &(argv[i][11]));
      }
      else
      {
        strcat(fname, argv[i]);
      }
    }
  }
  strcat(fname, ext);
  //Inicio los arrays de estadísticas
  threadCount = thCount;
  xactCount = xCount;
  //printf("thCount = %d, xCount = %d\n", threadCount, xactCount);

  stats = (struct Stats **)malloc(sizeof(struct Stats *) * thCount);
  if (!stats)
    return 0;
  for (i = 0; i < thCount; i++)
  {
    stats[i] = (struct Stats *)malloc(sizeof(struct Stats) * xactCount);
    if (!stats[i])
      return 0;
  }

  for (i = 0; i < thCount; i++)
  {
    for (j = 0; j < xactCount; j++)
    {
      stats[i][j].xabortCount = 0;
      stats[i][j].explicitAborts = 0;
      stats[i][j].explicitAbortsSubs = 0;
      stats[i][j].retryAborts = 0;
      stats[i][j].retryCapacityAborts = 0;
      stats[i][j].retryConflictAborts = 0;
      stats[i][j].conflictAborts = 0;
      stats[i][j].capacityAborts = 0;
      stats[i][j].debugAborts = 0;
      stats[i][j].nestedAborts = 0;
      stats[i][j].eaxzeroAborts = 0;
      stats[i][j].xcommitCount = 0;
      stats[i][j].fallbackCount = 0;
      stats[i][j].retryCCount = 0;
      stats[i][j].retryFCount = 0;
    }
  }

  return 1;
}

int dumpStats()
{
  FILE *f;
  int i, j;
  unsigned long int tmp, comm, fall, retComm;

  //Creo el fichero
  f = fopen(fname, "w");
  if (!f)
    return 0;
  printf("Writing TM stats to: %s\n", fname);
  fprintf(f, "-----------------------------------------\nOutput file: %s\n----------------- Stats -----------------\n", fname);
  fprintf(f, "#Threads: %li\n", threadCount);

  fprintf(f, "Abort Count:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].xabortCount)
      fprintf(f, "%lu ", stats[i][j].xabortCount);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "Explicit aborts:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].explicitAborts)
      fprintf(f, "%lu ", stats[i][j].explicitAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "  »     Subs:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].explicitAbortsSubs)
      fprintf(f, "%lu ", stats[i][j].explicitAbortsSubs);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "Retry aborts:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].retryAborts)
      fprintf(f, "%lu ", stats[i][j].retryAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "  »     Con:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].retryConflictAborts)
      fprintf(f, "%lu ", stats[i][j].retryConflictAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "  »     Cap:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].retryCapacityAborts)
      fprintf(f, "%lu ", stats[i][j].retryCapacityAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "Conflict aborts:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].conflictAborts)
      fprintf(f, "%lu ", stats[i][j].conflictAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "Capacity aborts:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].capacityAborts)
      fprintf(f, "%lu ", stats[i][j].capacityAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "Debug aborts:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].debugAborts)
      fprintf(f, "%lu ", stats[i][j].debugAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "Nested aborts:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].nestedAborts)
      fprintf(f, "%lu ", stats[i][j].nestedAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "eax=0 aborts:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].eaxzeroAborts)
      fprintf(f, "%lu ", stats[i][j].eaxzeroAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "Commits:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].xcommitCount)
      fprintf(f, "%lu ", stats[i][j].xcommitCount);
  }
  fprintf(f, " Total: %lu\n", tmp);
  comm = tmp;

  fprintf(f, "Fallbacks:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].fallbackCount)
      fprintf(f, "%lu ", stats[i][j].fallbackCount);
  }
  fprintf(f, " Total: %lu\n", tmp);
  fall = tmp;

  fprintf(f, "RetriesCommited:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].retryCCount)
      fprintf(f, "%lu ", stats[i][j].retryCCount);
  }
  fprintf(f, "Total: %lu ", tmp);
  retComm = tmp;
  fprintf(f, "PerXact: %f\n", (float)tmp / (float)comm);

  fprintf(f, "RetriesFallbacked:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].retryFCount)
      fprintf(f, "%lu ", stats[i][j].retryFCount);
  }
  fprintf(f, "Total: %lu ", tmp);
  fprintf(f, "PerXact: %f\nRetriesAvg: %f\n", (float)tmp / (float)fall,
          (float)(retComm + tmp) / (float)(comm + fall));

  fclose(f);
  for (i = 0; i < threadCount; i++)
    free(stats[i]);
  free(stats);
  return 1;
}

  void Barrier_init() {
  sense = 0;
  count = 0;
  pthread_mutex_init(&bar_lock, NULL);
}

void Barrier_non_breaking(int* local_sense, int id, int num_thr) {
  volatile int ret;

  if ((*local_sense) == 0)
    (*local_sense) = 1;
  else
    (*local_sense) = 0;

  pthread_mutex_lock(&bar_lock);
  count++;
  ret = (count == num_thr);
  pthread_mutex_unlock(&bar_lock);

  if (ret) {
    count = 0;
    sense = (*local_sense);
  } else {
    while (sense != (*local_sense)) {
      //usleep(1);     // For non-simulator runs
    }
  }
}

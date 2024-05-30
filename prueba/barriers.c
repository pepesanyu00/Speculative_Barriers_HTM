#include "barriers.h"


unsigned long barrierCounter = 0;

struct TicketLock g_ticketlock = {.ticket = 0, .turn = 1};
//fback_lock_t g_fallback_lock = {.ticket = 0, .turn = 1};
g_spec_vars_t g_specvars = {.tx_order = 1};

char fname[256];
long threadCount;
long xactCount;
struct Stats **stats;


int statsFileInit(int argc, char **argv, long thCount, long xCount)
{
  int i, j;
  char ext[25];

  //Saco la extensión con identificador de proceso para tener un archivo único
  sprintf(ext,"stats/%d.stats", getpid());
  strncpy(fname, ext, sizeof(fname) - 1);
  printf("Nombre del fichero: %s\n",fname);
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
      stats[i][j].xbeginCount = 0;
    }
  }

  return 1;
}

int dumpStats(double time)
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
  fprintf(f, "time: %f\n", time);

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

  fprintf(f, "  »     Conflict:");
  for (j = 0, tmp = 0; j < xactCount; j++)
  {
    fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].retryConflictAborts)
      fprintf(f, "%lu ", stats[i][j].retryConflictAborts);
  }
  fprintf(f, " Total: %lu\n", tmp);

  fprintf(f, "  »     Capacity:");
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


// Función para determinar el tipo de aborto
unsigned long profileAbortStatus(unsigned long eax, long thread, long xid)
{
  stats[thread][xid].xabortCount++;
  if (eax & _XABORT_EXPLICIT)
  {
    stats[thread][xid].explicitAborts++;
    if (_XABORT_CODE(eax) == LOCK_TAKEN)
      stats[thread][xid].explicitAbortsSubs++;
  }
  if (eax & _XABORT_RETRY)
  {
    stats[thread][xid].retryAborts++;
    if (eax & _XABORT_CONFLICT)
      stats[thread][xid].retryConflictAborts++;
    if (eax & _XABORT_CAPACITY)
      stats[thread][xid].retryCapacityAborts++;
    if (eax & _XABORT_DEBUG)
      assert(0);
    if (eax & _XABORT_NESTED)
      assert(0);
  }
  if (eax & _XABORT_CONFLICT)
  {
    stats[thread][xid].conflictAborts++;
  }
  if (eax & _XABORT_CAPACITY)
  {
    stats[thread][xid].capacityAborts++;
  }
  if (eax & _XABORT_DEBUG)
  {
    stats[thread][xid].debugAborts++;
  }
  if (eax & _XABORT_NESTED)
  {
    stats[thread][xid].nestedAborts++;
  }
  if (eax == 0)
  {
    //Todos los bits a cero (puede ocurrir por una llamada a CPUID u otro cosa)
    //Véase Section 8.3.5 RTM Abort Status Definition del Intel Architecture
    //Instruction Set Extensions Programming Reference (2012))
    stats[thread][xid].eaxzeroAborts++;
  }
  return 0;
}

// Función para indicar un commit
void profileCommit(long thread, long xid, long retries)
{
  stats[thread][xid].xcommitCount++;
  stats[thread][xid].retryCCount += retries;
}

// Funcion para indicar un fallback
void profileFallback(long thread, long xid, long retries)
{
  stats[thread][xid].fallbackCount++;
  stats[thread][xid].retryFCount += retries;
}
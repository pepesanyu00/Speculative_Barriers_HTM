#include "tmPower.h"

volatile uint32_t g_lock_var = 0;
fback_lock_t g_fallback_lock = {.ticket = 0, .turn = 1};
g_spec_vars_t g_specvars = {.tx_order = 1};

//RIC Archivo de estadísticas
char fname[256];
long threadCount;
struct Stats
{
  char pad1[CACHE_BLOCK_SIZE];             //Pads para que no haya tráfico que no coincidan xabortCount de un thread con xcommitCount the otro en un bloque cache
  unsigned long int xabortCount;           //Número total de abortos
  unsigned long int explicitAborts;        //Número de llamadas a XABORT en el código
  unsigned long int explicitAbortsSubs;    //Número de abortos explícitos por subscripción de lock
  unsigned long int explicitAbortsAddPath; //Número de abortos explícitos por subscripción de lock
  unsigned long int persistentAborts;      //Abortos para los que el hardware piensa que debemos reintentar
  unsigned long int disallowedAborts;      //Abortos por conflicto
  unsigned long int nestingAborts;         //Abortos por capacidad
  unsigned long int footprintAborts;       //Abortos por capacidad
  unsigned long int selfInducedAborts;     //Abortos por breakpoint de debugger
  unsigned long int nontransactAborts;     //Abortos dentro de una transacción anidada
  unsigned long int transactAborts;        //Abortos con eax = 0
  unsigned long int tlbAborts;             //Abortos con eax = 0
  unsigned long int implementationAborts;  //Abortos con eax = 0
  unsigned long int fetchAborts;           //Abortos con eax = 0
  unsigned long int otherAborts;           //Número de commits
  unsigned long int xcommitCount;          //Número de commits
  unsigned long int fallbackCount;         //Número de fallbacks
  unsigned long int retryCCount;           //Número de retries de las commitan
  unsigned long int retryFCount;           //Número de retries de las entran en fallback
  char pad2[CACHE_BLOCK_SIZE];
} * *stats;

void checkBuiltins()
{
  //#ifdef __BUILTIN_CPU_SUPPORTS__
  cout << "----------------- Builtin Check ----------------------------" << endl;
  if (__builtin_cpu_is("power8"))
    cout << "Builtin CPU is POWER8." << endl;
  if (__builtin_cpu_is("power9"))
    cout << "builtin CPU is POWER9." << endl;
  if (__builtin_cpu_supports("altivec"))
    cout << "CPU supports altivec." << endl;
  if (__builtin_cpu_supports("arch_2_07"))
    cout << "CPU supports ISA 2.07 (POWER8)." << endl;
  if (__builtin_cpu_supports("arch_3_00"))
    cout << "CPU supports ISA 3.0 (POWER9)." << endl;
  if (__builtin_cpu_supports("htm"))
    cout << "CPU has hardware transaction memory instructions." << endl;
  if (__builtin_cpu_supports("htm-nosc"))
    cout << "Kernel aborts hardware transactions when a syscall is made." << endl;
  if (__builtin_cpu_supports("htm-no-suspend"))
    cout << "CPU supports hardware transaction memory but does not support the tsuspend instruction." << endl;
  else
    cout << "CPU supports the tsuspend instruction." << endl;
  if (__builtin_cpu_supports("smt"))
    cout << "CPU supports simultaneous multi-threading." << endl;
  cout << "------------------------------------------------------------" << endl;
    
  //#else
  //cout << "Builtin functions not fully supported." << endl;
  //#endif
}

int statsFileInit(int argc, char **argv, long thCount)
{
  int i, j;
  char ext[25];

  checkBuiltins();

  //Saco la extensión con identificador de proceso para tener un archivo único
  sprintf(ext, "-%d.stats", getpid());
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

  //stats = (struct Stats **)malloc(sizeof(struct Stats *)*thCount);
  stats = new struct Stats *[thCount];
  if (!stats)
    return 0;
  for (i = 0; i < thCount; i++)
  {
    //stats[i] = (struct Stats *)malloc(sizeof(struct Stats)*MAX_XACT_IDS);
    stats[i] = new struct Stats[MAX_XACT_IDS];
    if (!stats[i])
      return 0;
  }

  for (i = 0; i < thCount; i++)
  {
    for (j = 0; j < MAX_XACT_IDS; j++)
    {
      stats[i][j].xabortCount = 0;
      stats[i][j].explicitAborts = 0;
      stats[i][j].explicitAbortsSubs = 0;
      stats[i][j].explicitAbortsAddPath = 0;
      stats[i][j].persistentAborts = 0;
      stats[i][j].disallowedAborts = 0;
      stats[i][j].nestingAborts = 0;
      stats[i][j].footprintAborts = 0;
      stats[i][j].selfInducedAborts = 0;
      stats[i][j].nontransactAborts = 0;
      stats[i][j].transactAborts = 0;
      stats[i][j].tlbAborts = 0;
      stats[i][j].implementationAborts = 0;
      stats[i][j].fetchAborts = 0;
      stats[i][j].otherAborts = 0;
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

  printf("Writing stats to: %s\n", fname);
  //Creo el fichero
  f = fopen(fname, "w");
  if (!f)
    return 0;
  fprintf(f, "-----------------------------------------\nOutput file: %s\n----------------- Stats -----------------\n", fname);
  fprintf(f, "#Threads: %li\n", threadCount);

  fprintf(f, "\nAbort Count: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].xabortCount)
      fprintf(f, "%lu ", stats[i][j].xabortCount);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "Explicit aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].explicitAborts)
      fprintf(f, "%lu ", stats[i][j].explicitAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "  »     Subs: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].explicitAbortsSubs)
      fprintf(f, "%lu ", stats[i][j].explicitAbortsSubs);
  }
  fprintf(f, "Total: %lu\n", tmp);

/*  fprintf(f, "  »  AddPath: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].explicitAbortsAddPath)
      fprintf(f, "%lu ", stats[i][j].explicitAbortsAddPath);
  }
  fprintf(f, "Total: %lu\n", tmp);
*/

  fprintf(f, "-Disallowed aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].disallowedAborts)
      fprintf(f, "%lu ", stats[i][j].disallowedAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "-Nesting overflow aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].nestingAborts)
      fprintf(f, "%lu ", stats[i][j].nestingAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "-Footprint overflow aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].footprintAborts)
      fprintf(f, "%lu ", stats[i][j].footprintAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "-Self-induced conflict aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].selfInducedAborts)
      fprintf(f, "%lu ", stats[i][j].selfInducedAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "  » Persistent: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].persistentAborts)
      fprintf(f, "%lu ", stats[i][j].persistentAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "Non-transactional conflict aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].nontransactAborts)
      fprintf(f, "%lu ", stats[i][j].nontransactAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "Transactional conflict aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].transactAborts)
      fprintf(f, "%lu ", stats[i][j].transactAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "TLB invalidation aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].tlbAborts)
      fprintf(f, "%lu ", stats[i][j].tlbAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "Implementation specific aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].implementationAborts)
      fprintf(f, "%lu ", stats[i][j].implementationAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "Instruction fetch conflict aborts: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0, tmp = 0; i < threadCount; tmp += stats[i++][j].fetchAborts)
      fprintf(f, "%lu ", stats[i][j].fetchAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "Other aborts (deberían ser 0): ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].otherAborts)
      fprintf(f, "%lu ", stats[i][j].otherAborts);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fprintf(f, "Commits: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].xcommitCount)
      fprintf(f, "%lu ", stats[i][j].xcommitCount);
  }
  fprintf(f, "Total: %lu\n", tmp);

  comm = tmp;
  fprintf(f, "Fallbacks: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].fallbackCount)
      fprintf(f, "%lu ", stats[i][j].fallbackCount);
  }
  fprintf(f, "Total: %lu\n", tmp);

  fall = tmp;
  fprintf(f, "RetriesCommited: ");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].retryCCount)
      fprintf(f, "%lu ", stats[i][j].retryCCount);
  }
  fprintf(f, "Total: %lu ", tmp);
  retComm = tmp;
  fprintf(f, "PerXact: %f\n", (float)tmp / (float)comm);

  fprintf(f, "RetriesFallbacked:");
  for (j = 0, tmp = 0; j < MAX_XACT_IDS; j++)
  {
    (j == SPEC_XACT_ID) ? fprintf(f, " XIDSB: ") : fprintf(f, " XID%d: ", j);
    for (i = 0; i < threadCount; tmp += stats[i++][j].retryFCount)
      fprintf(f, "%lu ", stats[i][j].retryFCount);
  }
  fprintf(f, "Total: %lu ", tmp);
  fprintf(f, "PerXact: %f\nRetriesAvg: %f", (float)tmp / (float)fall,
          (float)(retComm + tmp) / (float)(comm + fall));
  fclose(f);
  //free(stats);
  delete[] stats;
  return 1;
}

void profileCommit(long thread, long xid, long retries)
{
  stats[thread][xid].xcommitCount++;
  stats[thread][xid].retryCCount += retries;
}

void profileFallback(long thread, long xid, long retries)
{
  stats[thread][xid].fallbackCount++;
  stats[thread][xid].retryFCount += retries;
}

//La modifico para que devuelva si fue VALIDATION_ERROR
unsigned long profileAbortStatus(texasru_t cause, long thread, long xid)
{
  stats[thread][xid].xabortCount++;
  if (_TEXASRU_ABORT(cause))
  {
    stats[thread][xid].explicitAborts++;
    if (_TEXASRU_FAILURE_CODE(cause) == LOCK_TAKEN)
      stats[thread][xid].explicitAbortsSubs++;
    else if (_TEXASRU_FAILURE_CODE(cause) == VALIDATION_ERROR)
    {
      stats[thread][xid].explicitAbortsAddPath++;
      return 1;
    }
  }
  else if (_TEXASRU_DISALLOWED(cause))
  {
    stats[thread][xid].disallowedAborts++;
    if (_TEXASRU_FAILURE_PERSISTENT(cause))
      stats[thread][xid].persistentAborts++;
  }
  else if (_TEXASRU_NESTING_OVERFLOW(cause))
  {
    stats[thread][xid].nestingAborts++;
    if (_TEXASRU_FAILURE_PERSISTENT(cause))
      stats[thread][xid].persistentAborts++;
  }
  else if (_TEXASRU_FOOTPRINT_OVERFLOW(cause))
  {
    stats[thread][xid].footprintAborts++;
    if (_TEXASRU_FAILURE_PERSISTENT(cause))
      stats[thread][xid].persistentAborts++;
  }
  else if (_TEXASRU_SELF_INDUCED_CONFLICT(cause))
  {
    stats[thread][xid].selfInducedAborts++;
    if (_TEXASRU_FAILURE_PERSISTENT(cause))
      stats[thread][xid].persistentAborts++;
  }
  else if (_TEXASRU_NON_TRANSACTIONAL_CONFLICT(cause))
  {
    stats[thread][xid].nontransactAborts++;
  }
  else if (_TEXASRU_TRANSACTION_CONFLICT(cause))
  {
    stats[thread][xid].transactAborts++;
  }
  else if (_TEXASRU_TRANSLATION_INVALIDATION_CONFLICT(cause))
  {
    stats[thread][xid].tlbAborts++;
  }
  else if (_TEXASRU_IMPLEMENTAION_SPECIFIC(cause))
  {
    stats[thread][xid].implementationAborts++;
  }
  else if (_TEXASRU_INSTRUCTION_FETCH_CONFLICT(cause))
  {
    stats[thread][xid].fetchAborts++;
  }
  else
  {
    stats[thread][xid].otherAborts++;
  }
  return 0;
}
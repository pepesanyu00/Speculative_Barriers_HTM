#include <iostream>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <assert.h>
#include <omp.h>
#include <unistd.h> //For getpid(), used to get the pid to generate a unique filename
#include <typeinfo> //To obtain type name as string
#include <array>
#include <assert.h> //RIC incluyo assert para hacer comprobaciones de invariantes y condiciones
#include <immintrin.h>

#define PATH_RESULTS "./results/" /* RIC Quito SCAMP_ para hacer que se imprima el nombre del programa llamado como nombre del fichero de salida*/
#define DTYPE double              /* DATA TYPE */
#define ITYPE long long int       /* INDEX TYPE: RIC pongo long long int para que tanto el double como el int sean de 64 bits (facilita la vectorización) */

/*******************************************************************/
#if defined(AVX512)
/*******************************************************************/
#define ALIGN 64 // RIC 32 bytes para AVX2, 64 bytes para AVX512
#define VPACK 8  // Cuatro doubles empaquetados en un registro AVX2
#define VDTYPE __m512d
#define VITYPE __m512i
#define VMTYPE __mmask8

#define SETZERO_PD() _mm512_setzero_pd()
#define SET1_PD(a) _mm512_set1_pd(a)
#define SET1_EPI(a) _mm512_set1_epi64(a)
#define LOADU_PD(a) _mm512_loadu_pd(a)
#define LOADU_SI(a) _mm512_loadu_si512(a)
#define STORE_PD(a,b) _mm512_store_pd(a,b)
#define STOREU_PD(a,b) _mm512_storeu_pd(a,b)
#define STORE_SI(a,b) _mm512_store_si512((VITYPE *)a,b)
#define STOREU_SI(a,b) _mm512_storeu_si512((VITYPE *)a,b)
#define FMADD_PD(a,b,c) _mm512_fmadd_pd(a,b,c)
#define SUB_PD(a,b) _mm512_sub_pd(a,b)
#define ADD_PD(a,b) _mm512_add_pd(a,b)
#define MUL_PD(a,b) _mm512_mul_pd(a,b)
/* En AVX512 han añadido mask al final de esta instrucción
   (API más homogéneo). Han creado un tipo especial para la máscara
   __mmask8.
   */
#define CMP_PD(a,b,c) _mm512_cmp_pd_mask(a,b,c)
// En AVX512 la máscara va al principio
#define BLEND_EPI(a,b,c) _mm512_mask_blend_epi64(c,a,b)
#define BLEND_PD(a,b,c) _mm512_mask_blend_pd(c,a,b)
#define MASKSTOREU_PD(a,b,c) _mm512_mask_storeu_pd(a,b,c) // AVX512 diferencia mask stores unaligned y aligned
#define MASKSTOREU_EPI(a,b,c) _mm512_mask_storeu_epi64(a,b,c)
/*******************************************************************/
#elif defined(AVX2)
/*******************************************************************/
#define ALIGN 32 // RIC 32 bytes para AVX2, 64 bytes para AVX512
#define VPACK 4  // Cuatro doubles empaquetados en un registro AVX2
#define VDTYPE __m256d
#define VITYPE __m256i
#define VMTYPE VITYPE

#define SETZERO_PD() _mm256_setzero_pd()
#define SET1_PD(a) _mm256_set1_pd(a)
#define SET1_EPI(a) _mm256_set1_epi64x(a)
#define LOADU_PD(a) _mm256_loadu_pd(a)
#define LOADU_SI(a) _mm256_loadu_si256(a)
#define STORE_PD(a,b) _mm256_store_pd(a,b)
#define STOREU_PD(a,b) _mm256_storeu_pd(a,b)
#define STORE_SI(a,b) _mm256_store_si256((VITYPE *)a,b)
#define STOREU_SI(a,b) _mm256_storeu_si256((VITYPE *)a,b)
#define FMADD_PD(a,b,c)  _mm256_fmadd_pd(a,b,c)
#define SUB_PD(a,b) _mm256_sub_pd(a,b)
#define ADD_PD(a,b) _mm256_add_pd(a,b)
#define MUL_PD(a,b) _mm256_mul_pd(a,b)
/* En AVX2 esta intrucción devuelve una máscara de tipo __m256d.
   Hago casting a __m256i para usar en las instrucciones de blend.
   */
#define CMP_PD(a,b,c) (VMTYPE)_mm256_cmp_pd(a,b,c)
/* En AVX512 ponen funciones para cada tipo entero.
   En AVX2 sólo dan 1 instrucción para trabajar con el entero más pequeño
   (8 bytes). Las máscaras determinarán si se mueven 8 bytes o más.
   */
#define BLEND_EPI(a,b,c) _mm256_blendv_epi8(a,b,c)
#define BLEND_PD(a,b,c) _mm256_blendv_pd(a,b,(VDTYPE)c)
#define MASKSTOREU_PD(a,b,c) _mm256_maskstore_pd(a,b,c);  // En AVX2 parece que sólo hay maskstore y soporta unaligned y aligned
#define MASKSTOREU_EPI(a,b,c) _mm256_maskstore_epi64(a,b,c);

/*******************************************************************/
#else
/*******************************************************************/
#error "Tiene que definir la variable AVX512 o AVX2 al compilar."
#endif

//RIC Me defino unas macros para reservar memoria alineada
// Uso el operador ## para concatenar algo al nombre de la variable.
// Así creo dos variables al reservar memoria: la que se usará (alineada) y otra para que utilizaré al final para liberar memoria
#define ALIGNED_ARRAY_NEW(_type, _var, _elem, _align)                                                                                             \
  assert(_align >= sizeof(_type) && _elem >= 1);                          /* Compruebo condiciones */                                             \
                                                                          /* Reservo más elementos que elem: align(en bytes)/numbytes de type */ \
  _type *_var##__unaligned = new _type[_elem + _align / sizeof(_type)](); /* Con () inicializamos a 0*/                                           \
  assert(_var##__unaligned != NULL && _var##__unaligned[0] == 0 && _var##__unaligned[1] == 0);                                                    \
  /* Hago un casting del puntero con uintptr_t. De esta manera el operador + lo tomará como un número y operará en */                          \
  /* aritmética entera. Si no hiciera el casting, el compilador aplica aritmética de punteros */                                                \
  /* Luego hago una máscara con todo unos menos log2(align) ceros y dejo los lsb a 0 */                                                          \
  _var = (_type *)(((uintptr_t)_var##__unaligned + _align - 1) & ~(uintptr_t)(_align - 1));                                                       \
  assert(((uintptr_t)_var & (uintptr_t)(_align - 1)) == 0); /* Compruebo que var esté alineado */                                                \
  /* cout << #_var << "__unaligned: " << hex << _var##__unaligned << "(" << dec << (uintptr_t) _var##__unaligned << ") -> " << #_var << ": " << hex << _var << "(" << dec << (uintptr_t) _var << ")" << endl; */

#define ALIGNED_ARRAY_DEL(_var)      \
  assert(_var##__unaligned != NULL); \
  delete[] _var##__unaligned;

using namespace std;

/* ------------------------------------------------------------------ */

ITYPE numThreads, exclusionZone, windowSize, tSeriesLength, profileLength;
ITYPE debug = 1;

// Computes all required statistics for SCAMP, populating info with these values
void preprocess(DTYPE *tSeries, DTYPE *means, DTYPE *norms, DTYPE *df, DTYPE *dg, ITYPE tSeriesLength)
{

  vector<DTYPE> prefix_sum(tSeriesLength);
  vector<DTYPE> prefix_sum_sq(tSeriesLength);

  // Calculates prefix sum and square sum vectors
  prefix_sum[0] = tSeries[0];
  prefix_sum_sq[0] = tSeries[0] * tSeries[0];
  for (ITYPE i = 1; i < tSeriesLength; ++i)
  {
    prefix_sum[i] = tSeries[i] + prefix_sum[i - 1];
    prefix_sum_sq[i] = tSeries[i] * tSeries[i] + prefix_sum_sq[i - 1];
  }

  // Prefix sum value is used to calculate mean value of a given window, taking last value
  // of the window minus the first one and dividing by window size
  means[0] = prefix_sum[windowSize - 1] / static_cast<DTYPE>(windowSize);
  for (ITYPE i = 1; i < profileLength; ++i)
    means[i] = (prefix_sum[i + windowSize - 1] - prefix_sum[i - 1]) / static_cast<DTYPE>(windowSize);

  DTYPE sum = 0;
  for (ITYPE i = 0; i < windowSize; ++i)
  {
    DTYPE val = tSeries[i] - means[0];
    sum += val * val;
  }
  norms[0] = sum;

  // Calculates L2-norms (euclidean norm, euclidean distance)
  for (ITYPE i = 1; i < profileLength; ++i)
    norms[i] = norms[i - 1] + ((tSeries[i - 1] - means[i - 1]) + (tSeries[i + windowSize - 1] - means[i])) *
                                  (tSeries[i + windowSize - 1] - tSeries[i - 1]);
  for (ITYPE i = 0; i < profileLength; ++i)
    norms[i] = 1.0 / sqrt(norms[i]);

  // Calculates df and dg vectors
  for (ITYPE i = 0; i < profileLength - 1; ++i)
  {
    df[i] = (tSeries[i + windowSize] - tSeries[i]) / 2.0;
    dg[i] = (tSeries[i + windowSize] - means[i + 1]) + (tSeries[i] - means[i]);
  }
}

void scamp(DTYPE *tSeries, DTYPE *means, DTYPE *norms, DTYPE *df, DTYPE *dg, DTYPE *profile, ITYPE *profileIndex)
{
  // Private structures
  // RIC las pongo alineadas tb
  //vector<DTYPE> profile_tmp(profileLength * numThreads);
  //vector<ITYPE> profileIndex_tmp(profileLength * numThreads);
  DTYPE *profile_tmp = NULL;
  ITYPE *profileIndex_tmp = NULL;
  //RIC sumo VPACK al profileLength
  ALIGNED_ARRAY_NEW(DTYPE, profile_tmp, (profileLength + VPACK) * numThreads, ALIGN);
  ALIGNED_ARRAY_NEW(ITYPE, profileIndex_tmp, (profileLength + VPACK) * numThreads, ALIGN);

#pragma omp parallel
  {
    // ITYPE
    ITYPE my_offset = omp_get_thread_num() * (profileLength + VPACK);

    // Private profile initialization
    for (ITYPE i = my_offset; i < (my_offset + profileLength); i++)
      profile_tmp[i] = -numeric_limits<DTYPE>::infinity();

// Go through diagonals
#pragma omp for schedule(dynamic)
    for (ITYPE diag = exclusionZone + 1; diag < profileLength; diag += VPACK)
    /* Cada iteración lleva VPACK elementos a la vez con las instrucciones vectoriales*/
    {
      //covariance = 0; RIC tengo que crear una variable AVX para la covarianza
      VDTYPE covariance_v = SETZERO_PD(); // 4 covarianzas empaquetadas
      /* En este bucle se realiza el producto vectorial de cada elemento de las ventanas
         que están en las posiciones diag y 0 (centrado en la media, de ahí el - mean()
         En nuestro caso, realizaremos el producto vectorial de la [diag, diag+1, diag+2, diag+3]
         por la 0, en paralelo, vectorizando.
      */
      for (ITYPE i = 0; i < windowSize; i++)
      {
        // covariance += ((tSeries[diag + i] - means[diag]) * (tSeries[i] - means[0]));
        //assert(((uintptr_t)&tSeries[diag + i] & (uintptr_t)(ALIGN - 1)) == 0);
        VDTYPE tSeriesWinDiag_v = LOADU_PD(&tSeries[diag + i]); // Los afectados por diag se cargan de 4 en 4
        VDTYPE meansWinDiag_v = LOADU_PD(&means[diag]);
        VDTYPE tSeriesWin0_v = SET1_PD(tSeries[i]); // Los no afectados se replican
        VDTYPE meansWin0_v = SET1_PD(means[0]);
        // res = fma(a,b,c) -> res = a*b+c
        covariance_v = FMADD_PD(SUB_PD(tSeriesWinDiag_v, meansWinDiag_v), SUB_PD(tSeriesWin0_v, meansWin0_v), covariance_v);
      }

      ITYPE i = 0;
      ITYPE j = diag;
      // RIC La j es diag en realidad por lo que las normas j se cargan con load y las i con set1
      //correlation = covariance * norms[i] * norms[j];
      VDTYPE normsi_v = SET1_PD(norms[i]);
      VDTYPE normsj_v = LOADU_PD(&norms[j]);
      VDTYPE correlation_v = MUL_PD(MUL_PD(covariance_v, normsi_v), normsj_v);
      //double correlation = corr[0];
      //double covariance = 0;
      //RIC Este es el problema de encontrar el máximo horizontal
      /*if (correlation > profile_tmp[i + my_offset])
      {
        profile_tmp[i + my_offset] = correlation;
        profileIndex_tmp[i + my_offset] = j;
      }*/
      // RIC pruebo primero en serie. A realizar: vectorizar con el máximo horizontal y probar si los tiempos son mejores
      alignas(ALIGN) DTYPE correlation[VPACK];
      STORE_PD(correlation, correlation_v);
      for (int ii = 0; ii < VPACK; ii++)
      {
        if (correlation[ii] > profile_tmp[i + my_offset])
        {
          profile_tmp[i + my_offset] = correlation[ii];
          profileIndex_tmp[i + my_offset] = j + ii;
        }
      }
      // Para j, cargo los 4 profiles
      /*if (correlation > profile_tmp[j + my_offset])
      {
        profile_tmp[j + my_offset] = correlation;
        profileIndex_tmp[j + my_offset] = i;
      }*/
      VDTYPE profilej_v = LOADU_PD(&profile_tmp[j + my_offset]);
      VMTYPE mask = CMP_PD(correlation_v, profilej_v, _CMP_GT_OQ);
      MASKSTOREU_PD(&profile_tmp[j + my_offset], mask, correlation_v);
      MASKSTOREU_EPI(&profileIndex_tmp[j + my_offset], mask, SET1_EPI(i));

      i = 1;
      for (ITYPE j = diag + 1; j < profileLength; j++)
      {
        // esto se puede paralelizar
        // empieza por i = 0, j = diag y continúa
        //covariance += (df[i - 1] * dg[j - 1] + df[j - 1] * dg[i - 1]); // paralelizable
        // RIC seguimos con que las js son diag y son las que se empaquetan, las ies se replican
        VDTYPE dfj_v = LOADU_PD(&df[j - 1]); // Los afectados por diag se cargan de 4 en 4
        VDTYPE dgj_v = LOADU_PD(&dg[j - 1]);
        VDTYPE dfi_v = SET1_PD(df[i - 1]); // Los no afectados se replican
        VDTYPE dgi_v = SET1_PD(dg[i - 1]);
        // res = fma(a,b,c) -> res = a*b+c
        covariance_v = ADD_PD(covariance_v, FMADD_PD(dfi_v, dgj_v, MUL_PD(dfj_v, dgi_v)));

        //correlation = covariance * norms[i] * norms[j];                // mas complicado
        //RIC como antes, lo que va afectado de i se replica y lo que va afectado de j se carga
        normsi_v = SET1_PD(norms[i]);
        normsj_v = LOADU_PD(&norms[j]);
        correlation_v = MUL_PD(MUL_PD(covariance_v, normsi_v), normsj_v);

        // guardar el resultado mayor de la paralelizacion en variables auxiliares
        // y actualizar
        /*if (correlation > profile_tmp[i + my_offset])
        {
          profile_tmp[i + my_offset] = correlation;
          profileIndex_tmp[i + my_offset] = j;
        }*/
        STORE_PD(correlation, correlation_v);
        for (int ii = 0; ii < VPACK; ii++)
        {
          if (correlation[ii] > profile_tmp[i + my_offset])
          {
            profile_tmp[i + my_offset] = correlation[ii];
            profileIndex_tmp[i + my_offset] = j + ii;
          }
        }

        /*if (correlation > profile_tmp[j + my_offset])
        {
          profile_tmp[j + my_offset] = correlation;
          profileIndex_tmp[j + my_offset] = i;
        }*/
        profilej_v = LOADU_PD(&profile_tmp[j + my_offset]);
        mask = CMP_PD(correlation_v, profilej_v, _CMP_GT_OQ);
        MASKSTOREU_PD(&profile_tmp[j + my_offset], mask, correlation_v);
        MASKSTOREU_EPI(&profileIndex_tmp[j + my_offset], mask, SET1_EPI(i));

        i++;
      }

      /*
        Definitivamente no tiene sentido comparar con los elementos antes de diag, ya que la matriz
        va a ser siempre simétrica (la distancia del segmento a al b es la misma que la del b al a, en tér
        minos absolutos).
      */
    } //'pragma omp for' places here a barrier unless 'no wait' is specified

    //DTYPE max_corr;
    //ITYPE max_index = 0;
// Reduction. RIC Vectorizo el bucle externo (colum++ por colum+=VPACK)
#pragma omp for schedule(static)
    for (ITYPE colum = 0; colum < profileLength; colum += VPACK)
    {
      // max_corr = -numeric_limits<DTYPE>::infinity();
      VDTYPE max_corr_v = SET1_PD(-numeric_limits<DTYPE>::infinity());
      VITYPE max_indices_v;
      for (ITYPE th = 0; th < numThreads; th++)
      {
        // RIC Esta búsqueda del máximo sería como la segunda búsqueda del máximo del bucle anterior (la que va después de la que se hizo en serie)
        // Utilizo instrucciones del ejemplo max-v.cpp
        /*if (profile_tmp[colum + (th * (profileLength + VPACK))] > max_corr)
        {
          max_corr = profile_tmp[colum + (th * (profileLength + VPACK))];
          max_index = profileIndex_tmp[colum + (th * (profileLength + VPACK))];
        }*/
        // RIC Como el profileLength puede tener cualquier valor no puedo hacer los load alineados
        VDTYPE profile_tmp_v = LOADU_PD(&profile_tmp[colum + (th * (profileLength + VPACK))]);
        VITYPE profileIndex_tmp_v = LOADU_SI((VITYPE *)&profileIndex_tmp[colum + (th * (profileLength + VPACK))]);
        VMTYPE mask = CMP_PD(profile_tmp_v, max_corr_v, _CMP_GT_OQ);
        max_indices_v = BLEND_EPI(max_indices_v, profileIndex_tmp_v, mask); // Update con máscara de los índices
        max_corr_v = BLEND_PD(max_corr_v, profile_tmp_v, mask);     // Update con máscara de las correlaciones
        // RIC valdría tb con _mm256_max_pd (creo que no hay diferencia en el tiempo de ejecución)
        //max_corr_v = _mm256_max_pd(profile_tmp_v, max_corr_v);                        // Update con max de las correlaciones
      }
      //profile[colum] = max_corr;
      //profileIndex[colum] = max_index;
      //Los stores sí pueden ser alineados
      STORE_PD(&profile[colum], max_corr_v);
      STORE_SI(&profileIndex[colum], max_indices_v);
    }
  }

  ALIGNED_ARRAY_DEL(profile_tmp);
  ALIGNED_ARRAY_DEL(profileIndex_tmp);
}

int main(int argc, char *argv[])
{
  try
  {
    // Creation of time meassure structures
    chrono::steady_clock::time_point tstart, tend;
    chrono::duration<double> telapsed;

    if (argc != 4)
    {
      cout << "[ERROR] usage: ./scamp input_file win_size num_threads" << endl;
      return 0;
    }

    windowSize = atoi(argv[2]);
    numThreads = atoi(argv[3]);
    // Set the exclusion zone to 0.25
    exclusionZone = (ITYPE)(windowSize * 0.25);
    omp_set_num_threads(numThreads);

    //vector<DTYPE> tSeriesV; Usamos arrays
    string inputfilename = argv[1];
    stringstream tmp;
    // RIC Ahora permito que la timeseries se introduzca con el directorio
    // RIC Quito el directorio de la cadena de resultados con rfind('/') y meto el nombre del programa al principio del nombre del fichero de resultados
    tmp << PATH_RESULTS << argv[0] << "_" << inputfilename.substr(inputfilename.rfind('/') + 1, inputfilename.size() - 4 - inputfilename.rfind('/') - 1) << "_w" << windowSize << "_t" << numThreads << "_" << getpid() << ".csv";
    string outfilename = tmp.str();

    // Display info through console
    cout << endl;
    cout << "############################################################" << endl;
    cout << "///////////////////////// SCAMP ////////////////////////////" << endl;
    cout << "############################################################" << endl;
    cout << endl;
    cout << "[>>] Reading File: " << inputfilename << "..." << endl;

    /* ------------------------------------------------------------------ */
    /* Count file lines */
    tstart = chrono::steady_clock::now();

    fstream tSeriesFile(inputfilename, ios_base::in);
    tSeriesLength = 0;
    cout << "[>>] Counting lines ... " << endl;
    string line;
    while (getline(tSeriesFile, line)) // Cuento el número de líneas
      tSeriesLength++;

    tend = chrono::steady_clock::now();
    telapsed = tend - tstart;
    cout << "[OK] Lines: " << tSeriesLength << " Time: " << telapsed.count() << "s." << endl;
    /* ------------------------------------------------------------------ */
    /* Read time series file */
    cout << "[>>] Reading values ... " << endl;
    tstart = chrono::steady_clock::now();
    DTYPE *tSeries = NULL; // Defino la serie temporal como un puntero a DTYPE
    // Le sumo a la longitud el VPACK para que cuando se trabaje en los límites de la serie se cojan elementos reservados (aunque luego no sirvan los cálculos que se hacen con ellos)
    ALIGNED_ARRAY_NEW(DTYPE, tSeries, tSeriesLength + VPACK, ALIGN);
    tSeriesFile.clear();                // Limpio el stream
    tSeriesFile.seekg(tSeriesFile.beg); // Y lo reinicio a beginning
    DTYPE tempval, tSeriesMin = numeric_limits<DTYPE>::infinity(), tSeriesMax = -numeric_limits<double>::infinity();
    // Comprobar si el fichero tiene algún NaN y quitarlo, porque esto podría fallar
    for (int i = 0; tSeriesFile >> tempval; i++)
    {
      tSeries[i] = tempval;
      if (tempval < tSeriesMin)
        tSeriesMin = tempval;
      if (tempval > tSeriesMax)
        tSeriesMax = tempval;
    }
    tSeriesFile.close();
    tend = chrono::steady_clock::now();
    telapsed = tend - tstart;
    cout << "[OK] Time: " << telapsed.count() << "s." << endl;

    /* ------------------------------------------------------------------ */
    // Set Matrix Profile Length
    profileLength = tSeriesLength - windowSize + 1;

    // Defino los arrays con las macros creadas para que salgan alineados (aunque finalmente uso loadu, y probablemente no haga falta usarlas)
    // Sumo a la longitud VPACK para que en los límites se trabaje con memoria reservada aunque no se utilicen los datos
    DTYPE *norms = NULL, *means = NULL, *df = NULL, *dg = NULL, *profile = NULL;
    ITYPE *profileIndex = NULL;
    ALIGNED_ARRAY_NEW(DTYPE, norms, profileLength + VPACK, ALIGN);
    ALIGNED_ARRAY_NEW(DTYPE, means, profileLength + VPACK, ALIGN);
    ALIGNED_ARRAY_NEW(DTYPE, df, profileLength + VPACK, ALIGN);
    ALIGNED_ARRAY_NEW(DTYPE, dg, profileLength + VPACK, ALIGN);
    ALIGNED_ARRAY_NEW(DTYPE, profile, profileLength + VPACK, ALIGN);
    ALIGNED_ARRAY_NEW(ITYPE, profileIndex, profileLength + VPACK, ALIGN);
    
    // Display info through console
    cout << endl;
    cout << "------------------------------------------------------------" << endl;
    cout << "************************** INFO ****************************" << endl;
    cout << endl;
    cout << " Series/MP data type: " << typeid(tSeries[0]).name() << "(" << sizeof(tSeries[0]) << "B)" << endl;
    cout << " Index data type:     " << typeid(profileIndex[0]).name() << "(" << sizeof(profileIndex[0]) << "B)" << endl;
    cout << " Time series length:  " << tSeriesLength << endl;
    cout << " Window size:         " << windowSize << endl;
    cout << " Time series min:     " << tSeriesMin << endl;
    cout << " Time series max:     " << tSeriesMax << endl;
    cout << " Number of threads:   " << numThreads << endl;
    cout << " Exclusion zone:      " << exclusionZone << endl;
    cout << " Profile length:      " << profileLength << endl;
    cout << "------------------------------------------------------------" << endl;
    cout << endl;

    /***************** Preprocess ******************/
    cout << "[>>] Preprocessing..." << endl;
    tstart = chrono::steady_clock::now();
    preprocess(tSeries, means, norms, df, dg, tSeriesLength);
    tend = chrono::steady_clock::now();
    telapsed = tend - tstart;
    cout << "[OK] Preprocessing Time:         " << setprecision(numeric_limits<double>::digits10 + 2) << telapsed.count() << " seconds." << endl;
    /***********************************************/

    /******************** SCAMP ********************/
    cout << "[>>] Executing SCAMP..." << endl;
    tstart = chrono::steady_clock::now();
    scamp(tSeries, means, norms, df, dg, profile, profileIndex);
    tend = chrono::steady_clock::now();
    telapsed = tend - tstart;
    cout << "[OK] SCAMP Time:              " << setprecision(numeric_limits<double>::digits10 + 2) << telapsed.count() << " seconds." << endl;
    /***********************************************/

    cout << "[>>] Saving result: " << outfilename << " ..." << endl;
    fstream statsFile(outfilename, ios_base::out);
    statsFile << "# Time (s)" << endl;
    statsFile << setprecision(9) << telapsed.count() << endl;
    statsFile << "# Profile Length" << endl;
    statsFile << profileLength << endl;
    statsFile << "# i,tseries,profile,index" << endl;
    for (ITYPE i = 0; i < profileLength; i++)
      statsFile << i << "," << tSeries[i] << "," << (DTYPE)sqrt(2 * windowSize * (1 - profile[i])) << "," << profileIndex[i] << endl;
    statsFile.close();

    cout << endl;

    //Libero memoria
    ALIGNED_ARRAY_DEL(tSeries);
    ALIGNED_ARRAY_DEL(norms);
    ALIGNED_ARRAY_DEL(means);
    ALIGNED_ARRAY_DEL(df);
    ALIGNED_ARRAY_DEL(dg);
    ALIGNED_ARRAY_DEL(profile);
    ALIGNED_ARRAY_DEL(profileIndex);
  }
  catch (exception &e)
  {
    cout << "Exception: " << e.what() << endl;
  }
}

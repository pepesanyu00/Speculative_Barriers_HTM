#include <iostream>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <cassert>
#include <omp.h>
#include <unistd.h> //For getpid(), used to get the pid to generate a unique filename
#include <typeinfo> //To obtain type name as string

#define PATH_RESULTS "./results/"

#define DTYPE double        /* DATA TYPE */
#define ITYPE long long int /* INDEX TYPE */

#if defined(CG) /* global lock */
#define SYNC_BEGIN(lock) omp_set_lock(&(fg_locks[0]))

#define SYNC_END(lock) omp_unset_lock(&(fg_locks[0]))

#define INIT_SYNC(lock_count)   \
  fg_locks = new omp_lock_t[1]; \
  assert(fg_locks);             \
  for (ITYPE i = 0; i < 1; i++)   \
  omp_init_lock(&(fg_locks[i]))

#elif defined(FG) /* fine grain */
#define SYNC_BEGIN(lock) omp_set_lock(&(fg_locks[lock]))

#define SYNC_END(lock) omp_unset_lock(&(fg_locks[lock]))

#define INIT_SYNC(lock_count)            \
  fg_locks = new omp_lock_t[lock_count]; \
  assert(fg_locks);                      \
  for (ITYPE i = 0; i < lock_count; i++)   \
  omp_init_lock(&(fg_locks[i]))

#endif

using namespace std;

ITYPE numThreads, exclusionZone, windowSize, tSeriesLength, profileLength, xactLength;
omp_lock_t *fg_locks;

// Computes all required statistics for SCAMP, populating info with these values
void preprocess(vector<DTYPE> &tSeries, vector<DTYPE> &means, vector<DTYPE> &norms,
                vector<DTYPE> &df, vector<DTYPE> &dg)
{

  vector<DTYPE> prefix_sum(tSeries.size());
  vector<DTYPE> prefix_sum_sq(tSeries.size());

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

void scamp(vector<DTYPE> &tSeries, vector<DTYPE> &means, vector<DTYPE> &norms,
           vector<DTYPE> &df, vector<DTYPE> &dg, vector<DTYPE> &profile, vector<ITYPE> &profileIndex)
{
  //RIC con la memoria transaccional vamos a intentar no privatizar y acceder al profile y al indexProfile protegiéndolo con una transacción
  // Private structures
  //vector<DTYPE> profile_tmp(profileLength * numThreads);
  //vector<ITYPE> profileIndex_tmp(profileLength * numThreads);

#pragma omp parallel proc_bind(close)
  {
    // Suppossing ITYPE as uint32_t (we could index series up to 4G elements), to index profile_tmp we need more bits (uint64_t)
    //uint64_t my_offset = omp_get_thread_num() * profileLength;
    //ITYPE tid = omp_get_thread_num();
    DTYPE covariance, correlation;

// Private profile initialization
//for (uint64_t i = my_offset; i < (my_offset+profileLength); i++)
//  profile_tmp[i] = -numeric_limits<DTYPE>::infinity();

// Go through diagonals
#pragma omp for schedule(dynamic) nowait
    for (ITYPE diag = exclusionZone + 1; diag < profileLength; diag++)
    {
      covariance = 0;
      for (ITYPE i = 0; i < windowSize; i++)
        covariance += ((tSeries[diag + i] - means[diag]) * (tSeries[i] - means[0]));

      ITYPE i = 0;
      ITYPE j = diag;

      correlation = covariance * norms[i] * norms[j];

      SYNC_BEGIN(i);
      if (correlation > profile[i])
      {
        profile[i] = correlation; //Actúo sobre el array global
        profileIndex[i] = j;
      }
#if defined(FG) //Si se define finegrain cada lock protege un elemento del profile. Si es CG protego el acceso a ambos elementos
      SYNC_END(i);
      SYNC_BEGIN(j);
#endif
      if (correlation > profile[j])
      {
        profile[j] = correlation;
        profileIndex[j] = i;
      }
      SYNC_END(j);

      i = 1;
      for (ITYPE j = diag + 1; j < profileLength; j++)
      {
        covariance += (df[i - 1] * dg[j - 1] + df[j - 1] * dg[i - 1]);
        correlation = covariance * norms[i] * norms[j];

        SYNC_BEGIN(i);
        if (correlation > profile[i])
        {
          profile[i] = correlation;
          profileIndex[i] = j;
        }
#if defined(FG)
        SYNC_END(i);
        SYNC_BEGIN(j);
#endif
        if (correlation > profile[j])
        {
          profile[j] = correlation;
          profileIndex[j] = i;
        }
        SYNC_END(j);
        i++;
      }
    } //'pragma omp for' places here a barrier unless 'no wait' is specified
    //RIC ya no hace falta la reducción final
    /*DTYPE max_corr;
    ITYPE max_index = 0;
    // Reduction
    #pragma omp for schedule(static)
    for (ITYPE colum = 0; colum < profileLength; colum++) {
      max_corr = -numeric_limits<DTYPE>::infinity();
      for (ITYPE th = 0; th < numThreads; th++) { // uint64_t counter to promote the index of profile_tmp to uint64_t
        if (profile_tmp[colum + (th * profileLength)] > max_corr) {
          max_corr = profile_tmp[colum + (th * profileLength)];
          max_index = profileIndex_tmp[colum + (th * profileLength)];
        }
      }
      profile[colum] = max_corr;
      profileIndex[colum] = max_index;
    }*/
  }
}

int main(int argc, char *argv[])
{
  try
  {
    // Creation of time meassure structures
    chrono::steady_clock::time_point tstart, tend;
    chrono::duration<double> telapsed;

    if (argc != 5)
    {
      cout << "usage: " << argv[0] <<  " input_file win_size num_threads dump_profile" << endl;
      cout << "       - input_file: ./timeseries/<file_name> " << endl;
      cout << "       - win_size: number between 1 and tseries length - 1" << endl;
      cout << "       - num_threads: number of threads to spawn" << endl;
      cout << "       - dump_profile: 1 - dump the profile in the csv file; 0 - no dump" << endl;
      return 0;
    }

    windowSize = atoi(argv[2]);
    numThreads = atoi(argv[3]);
    bool dumpProfile = (atoi(argv[4]) == 0) ? false : true;
    // Set the exclusion zone to 0.25
    exclusionZone = (ITYPE)(windowSize * 0.25);
    omp_set_num_threads(numThreads);

    vector<DTYPE> tSeries;
    string inputfilename = argv[1];
    stringstream tmp;
    tmp << PATH_RESULTS << argv[0] << "_" << inputfilename.substr(inputfilename.rfind('/') + 1, inputfilename.size() - 4 - inputfilename.rfind('/') - 1) << "_w" << windowSize << "_t" << numThreads << "_d" << dumpProfile << "_" << getpid() << ".csv";
    string outfilename = tmp.str();

    // Display info through console
    cout << endl;
    cout << "############################################################" << endl;
    cout << "///////////////////////// " << argv[0] << " ////////////////////////////" << endl;
    cout << "############################################################" << endl;
    cout << endl;
    cout << "[>>] Reading File: " << inputfilename << "..." << endl;

    /* ------------------------------------------------------------------ */
    /* Read time series file */
    tstart = chrono::steady_clock::now();

    fstream tSeriesFile(inputfilename, ios_base::in);

    DTYPE tempval, tSeriesMin = numeric_limits<DTYPE>::infinity(), tSeriesMax = -numeric_limits<double>::infinity();

    tSeriesLength = 0;
    while (tSeriesFile >> tempval)
    {
      tSeries.push_back(tempval);

      if (tempval < tSeriesMin)
        tSeriesMin = tempval;
      if (tempval > tSeriesMax)
        tSeriesMax = tempval;
      tSeriesLength++;
    }
    tSeriesFile.close();

    tend = chrono::steady_clock::now();
    telapsed = tend - tstart;
    cout << "[OK] Read File Time: " << setprecision(2) << fixed << telapsed.count() << " seconds." << endl;

    // Set Matrix Profile Length
    profileLength = tSeriesLength - windowSize + 1;
    INIT_SYNC(profileLength);

    // Auxiliary vectors
    vector<DTYPE> norms(profileLength), means(profileLength), df(profileLength), dg(profileLength);
    vector<DTYPE> profile(profileLength);
    vector<ITYPE> profileIndex(profileLength);

    //Profile initialization
    for (ITYPE i = 0; i < profileLength; i++) {
      profile[i] = -numeric_limits<DTYPE>::infinity();
    }

    // Display info through console
    cout << endl;
    cout << "------------------------------------------------------------" << endl;
    cout << "************************** INFO ****************************" << endl;
    cout << endl;
    cout << " Series/MP data type: " << typeid(tSeries[0]).name() << "(" << sizeof(tSeries[0]) << "B)" << endl;
    cout << " Index data type:     " << typeid(profileIndex[0]).name() << "(" << sizeof(profileIndex[0]) << "B)" << endl;
    cout << " OMP lock size:       " << sizeof(omp_lock_t) << "B" << endl;
    cout << " Time series length:  " << tSeriesLength << endl;
    cout << " Window size:         " << windowSize << endl;
    cout << " Dump profile:        " << dumpProfile << endl;
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
    preprocess(tSeries, means, norms, df, dg);
    tend = chrono::steady_clock::now();
    telapsed = tend - tstart;
    cout << "[OK] Preprocessing Time:         " << setprecision(2) << fixed << telapsed.count() << " seconds." << endl;
    /***********************************************/

    /******************** SCAMP ********************/
    cout << "[>>] Executing SCAMP..." << endl;
    tstart = chrono::steady_clock::now();
    scamp(tSeries, means, norms, df, dg, profile, profileIndex);
    tend = chrono::steady_clock::now();
    telapsed = tend - tstart;
    cout << "[OK] SCAMP Time:              " << setprecision(2) << fixed << telapsed.count() << " seconds." << endl;
    /***********************************************/

    cout << "[>>] Saving result: " << outfilename << " ..." << endl;
    fstream statsFile(outfilename, ios_base::out);
    statsFile << "# Time (s)" << endl;
    statsFile << setprecision(6) << fixed << telapsed.count() << endl;
    // El tamaño del profile y del profileIndex no va multiplicado por (numthreads + 1) porque no hay privatización
    // Pero le sumo el tamaño del vector de locks en el caso FG a profile, por lo que profile - profileIndex = tamaño vector de locks
    statsFile << "# Mem(KB) tseries,means,norms,df,dg,profile,profileIndex,Total(MB)" << endl;
#if defined(FG)
    statsFile << setprecision(2) << fixed << (sizeof(DTYPE) * tSeries.size()) / 1024.0f << "," << (sizeof(DTYPE) * means.size()) / 1024.0f << "," << (sizeof(DTYPE) * norms.size()) / 1024.0f << "," << (sizeof(DTYPE) * df.size()) / 1024.0f << "," << (sizeof(DTYPE) * dg.size()) / 1024.0f << "," << (sizeof(DTYPE) * profile.size() + sizeof(omp_lock_t) * profileLength) / 1024.0f << "," << (sizeof(ITYPE) * profileIndex.size()) / 1024.0f << "," << ((sizeof(DTYPE) * tSeries.size()) / 1024.0f + (sizeof(DTYPE) * means.size()) / 1024.0f + (sizeof(DTYPE) * norms.size()) / 1024.0f + (sizeof(DTYPE) * df.size()) / 1024.0f + (sizeof(DTYPE) * dg.size()) / 1024.0f + (sizeof(DTYPE) * profile.size() + sizeof(omp_lock_t) * profileLength) / 1024.0f + (sizeof(ITYPE) * profileIndex.size()) / 1024.0f) / 1024.0f << endl;
#elif defined(CG)
    statsFile << setprecision(2) << fixed << (sizeof(DTYPE) * tSeries.size()) / 1024.0f << "," << (sizeof(DTYPE) * means.size()) / 1024.0f << "," << (sizeof(DTYPE) * norms.size()) / 1024.0f << "," << (sizeof(DTYPE) * df.size()) / 1024.0f << "," << (sizeof(DTYPE) * dg.size()) / 1024.0f << "," << (sizeof(DTYPE) * profile.size() + sizeof(omp_lock_t)) / 1024.0f << "," << (sizeof(ITYPE) * profileIndex.size()) / 1024.0f << "," << ((sizeof(DTYPE) * tSeries.size()) / 1024.0f + (sizeof(DTYPE) * means.size()) / 1024.0f + (sizeof(DTYPE) * norms.size()) / 1024.0f + (sizeof(DTYPE) * df.size()) / 1024.0f + (sizeof(DTYPE) * dg.size()) / 1024.0f + (sizeof(DTYPE) * profile.size() + sizeof(omp_lock_t)) / 1024.0f + (sizeof(ITYPE) * profileIndex.size()) / 1024.0f) / 1024.0f << endl;
#endif
    statsFile << "# Profile Length" << endl;
    statsFile << profileLength << endl;
    if (dumpProfile)
    {
      statsFile << "# i,tseries,profile,index" << endl;
      for (ITYPE i = 0; i < profileLength; i++)
      {
        statsFile << i << "," << setprecision(numeric_limits<DTYPE>::max_digits10) << tSeries[i] << "," << (DTYPE)sqrt(2 * windowSize * (1 - profile[i])) << "," << profileIndex[i] << endl;
      }
    }
    statsFile.close();

    cout << endl;
    return 0;
  }
  catch (exception &e)
  {
    cout << "Exception: " << e.what() << endl;
  }
}

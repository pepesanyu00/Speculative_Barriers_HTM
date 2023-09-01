/***************************************************************************
 *cr
 *cr            (C) Copyright 2010 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/

#include "common.h"
#include "tm.h"
#include "thread.h"

//void cpu_stencil(float c0, float c1, float *A0, float * Anext, const int nx, const int ny, const int nz) {

void kernel_stencil(void * p) {
  params_t *pp = (params_t *) p;
  int nx, ny, nz, nthreads, tid, chunk, start, stop;
  float c0, c1, *A0, *Anext;
  nx = pp->nx;
  ny = pp->ny;
  nz = pp->nz;
  nthreads = pp->nthreads;
  c0 = pp->c0;
  c1 = pp->c1;
  A0 = pp->A0;
  Anext = pp->Anext;
  tid = thread_getId();

  TM_THREAD_ENTER();
  chunk = (nx - 2 + nthreads - 1) / nthreads;
  start = tid*chunk + 1;
  stop = (start+chunk) > (nx - 1)? nx - 1 : start + chunk; //RIC al último se le asigna el final del vector
  //printf("tid: %d, start: %d, stop: %d\n", tid, start, stop);
  int t;
  /*int t;
  for (t = 0; t < iteration; t++) {
    cpu_stencil(c0, c1, h_A0, h_Anext, nx, ny, nz);
    float *temp = h_A0;
    h_A0 = h_Anext;
    h_Anext = temp;
  }*/
  for (t = 0; t < pp->iterations; t++) {
    //#pragma omp parallel for
    //for (i = 1; i < nx - 1; i++) {
    int i;
    for (i = start; i < stop; i++) {
      int j, k;
      for (j = 1; j < ny - 1; j++) {
        for (k = 1; k < nz - 1; k++) {
          //i      #pragma omp critical
          TM_BEGIN(tid, 0);
          Anext[Index3D(nx, ny, i, j, k)] =
                  (A0[Index3D(nx, ny, i, j, k + 1)] +
                  A0[Index3D(nx, ny, i, j, k - 1)] +
                  A0[Index3D(nx, ny, i, j + 1, k)] +
                  A0[Index3D(nx, ny, i, j - 1, k)] +
                  A0[Index3D(nx, ny, i + 1, j, k)] +
                  A0[Index3D(nx, ny, i - 1, j, k)]) * c1
                  - A0[Index3D(nx, ny, i, j, k)] * c0;
          TM_END(tid, 0);
        }
      }
    }
    TM_BARRIER(tid);
    float *temp = A0;
    A0 = Anext;
    Anext = temp;
  }
  TM_LAST_BARRIER(tid);
  TM_THREAD_EXIT();
}



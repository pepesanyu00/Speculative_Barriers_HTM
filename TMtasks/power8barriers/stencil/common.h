/***************************************************************************
 *cr
 *cr            (C) Copyright 2010 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_

//float c0, float c1, float *A0, float * Anext, const int nx, const int ny, const int nz
typedef struct {
  float c0, c1;
  float *A0, *Anext;
  int nx, ny, nz, iterations, nthreads;
} params_t;

#define Index3D(_nx,_ny,_i,_j,_k) ((_i)+_nx*((_j)+_ny*(_k)))
#endif

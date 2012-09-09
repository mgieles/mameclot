#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mameclot.h"

#define sqr(x) pow(x,2.0)

void calculate_potential(float *m, float *x, float *y, float *z, float *phi, int *np);

void calculate_potential(float *m,float *x, float *y, float *z, float *phi,int *np)
{
  double d;
  int N = *np;
  for (int i=0; i<N; i++){
    for (int j=i+1; j<N; j++){
      d = sqrt(sqr(x[i]-x[j]) +  sqr(y[i]-y[j]) +  sqr(z[i]-z[j]));
      phi[i] -= m[j]/d;
      phi[j] -= m[i]/d;
    }   
  }
}

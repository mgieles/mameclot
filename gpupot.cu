// Programer: Pablo Bena Llambay / Mark Gieles
// Date: 25/06/2012
// A short example: calculation of potential of many particles.
// Compilation: write nvcc potential.cu

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cuda.h>
#include "mameclot.h"

#define BLOCKSIZE 256
	
//Computing the Potential on the Device
__global__ void compute_potential_gpu(float *m, float *x, float *y, float *z, float *pot,int N) {
  int i,j; 
  float rij, rijx, rijy, rijz;
  float xi, yi, zi;
  float potential; // Be careful with float or singles variables!
  i = threadIdx.x + blockIdx.x*blockDim.x;
  if (i<N) {
    xi = x[i];
    yi = y[i];
    zi = z[i];
    potential = 0.0f;
    for (j=0; j<N; j++) {
      rijx = xi - x[j];
      rijy = yi - y[j];
      rijz = zi - z[j];
      if (j!=i){
      	 rij = -m[j]/sqrt(rijx*rijx + rijy*rijy + rijz*rijz);
	 potential += rij;
	 }	 	   
    }
    pot[i] = potential;
  }
}


extern "C" void calculate_potential(float *m, float *x, float *y, float *z, float *pot, int *np) 
{
  float *m_d,*x_d,*y_d,*z_d,*pot_d; // Device variables!

  int N=*np;

  //Allocating memory on the Device
  cudaMalloc(&m_d  , sizeof(float)*N); 

  cudaMalloc(&x_d  , sizeof(double)*N); 
  cudaMalloc(&y_d  , sizeof(float)*N);
  cudaMalloc(&z_d  , sizeof(float)*N); 
  cudaMalloc(&pot_d, sizeof(float)*N);

  cudaMemcpy(m_d,m    , sizeof(float)*N, cudaMemcpyHostToDevice); // Host -> Device
  cudaMemcpy(x_d,x    , sizeof(float)*N, cudaMemcpyHostToDevice); // Host -> Device
  cudaMemcpy(y_d,y    , sizeof(float)*N, cudaMemcpyHostToDevice); // Host -> Device
  cudaMemcpy(z_d,z    , sizeof(float)*N, cudaMemcpyHostToDevice); // Host -> Device
  cudaMemcpy(pot_d,pot, sizeof(float)*N, cudaMemcpyHostToDevice); // Host -> Device

  
  compute_potential_gpu <<<((N+BLOCKSIZE-1))/BLOCKSIZE,BLOCKSIZE >>>(m_d,x_d, y_d, z_d, pot_d,N);
  cudaMemcpy(pot,pot_d, sizeof(float)*N, cudaMemcpyDeviceToHost); // Host -> Device
    
  //Freeing memory
  cudaFree(m_d);
  cudaFree(x_d);
  cudaFree(y_d);
  cudaFree(z_d);
  cudaFree(pot_d);
  
  return;
}

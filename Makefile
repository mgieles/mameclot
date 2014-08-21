CC = gcc
CFLAGS = -std=c99 -Wall -W -O3
EXE = mameclot
SOURCE = $(EXE).c 
OBJ = $(EXE).o
CPUOBJ = pot.o
LIBS = -lm

CUDAPATH = /usr/local/cuda/bin/
CUDAFLAGS = -L/usr/local/cuda/lib64  -lcudart 
NVCC = $(CUDAPATH)/nvcc
CUDASOURCE = gpupot.cu
CUDAOBJ = gpupot.o


default:	$(OBJ) $(CPUOBJ) 
		$(CC)  -o $(EXE) pot.o $(OBJ) $(LIBS) 

gpu:	$(OBJ)
	$(NVCC) -c -m64   $(CUDASOURCE)
	$(CC)   -o $(EXE).gpu $(CUDAOBJ) $(OBJ) $(LIBS) $(CUDAFLAGS) 


clean:	
	rm -f *.o $(EXE) $(EXE).gpu



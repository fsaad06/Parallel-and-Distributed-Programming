# Makefile to compile and run matrix_mul
# compile: make
# run matrix_mul: make runA
# clean: make clean

all: MatrixMul

# Output targets
MatrixMul: matrix_mul.obj Server.obj Client.obj
	g++ matrix_mul.obj Server.obj Client.obj -fopenmp -pthread -o MatrixMul

# Removed standalone Client and Server targets

# Intermediate object files
matrix_mul.obj: matrix_mul.cpp Server.h Client.h
	g++ -c matrix_mul.cpp -fopenmp -pthread -o matrix_mul.obj

Client.obj: Client.cpp Client.h
	g++ -c Client.cpp -fopenmp -pthread -o Client.obj

Server.obj: Server.cpp Server.h
	g++ -c Server.cpp -fopenmp -pthread -o Server.obj

runA: MatrixMul
	./MatrixMul $(ARG)

clean:
	rm -rf *.obj MatrixMul
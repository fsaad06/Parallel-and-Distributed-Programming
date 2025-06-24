# Makefile to compile and run array_sum
# compile: make
# run array_sum: make runA
# clean: make clean

all: ArraySum

# Output targets
ArraySum: array_sum.obj Server.obj Client.obj
	g++ array_sum.obj Server.obj Client.obj -fopenmp -pthread -o ArraySum

# Removed standalone Client and Server targets

# Intermediate object files
array_sum.obj: array_sum.cpp Server.h Client.h
	g++ -c array_sum.cpp -fopenmp -pthread -o array_sum.obj

Client.obj: Client.cpp Client.h
	g++ -c Client.cpp -fopenmp -pthread -o Client.obj

Server.obj: Server.cpp Server.h
	g++ -c Server.cpp -fopenmp -pthread -o Server.obj

runA: ArraySum
	./ArraySum $(ARG)

clean:
	rm -rf *.obj ArraySum
all: Laplace

Laplace: laplace.obj Server.obj Client.obj
	g++ -o Laplace laplace.obj Server.obj Client.obj -fopenmp -pthread -I/usr/include/python3.12 -I/usr/lib/python3/dist-packages/numpy/core/include -lpython3.12

laplace.obj: laplace.cpp Server.h Client.h
	g++ -c laplace.cpp -fopenmp -pthread -I/usr/include/python3.12 -I/usr/lib/python3/dist-packages/numpy/core/include -o laplace.obj

Client.obj: Client.cpp Client.h
	g++ -c Client.cpp -fopenmp -pthread -I/usr/include/python3.12 -I/usr/lib/python3/dist-packages/numpy/core/include -o Client.obj

Server.obj: Server.cpp Server.h
	g++ -c Server.cpp -fopenmp -pthread -I/usr/include/python3.12 -I/usr/lib/python3/dist-packages/numpy/core/include -o Server.obj

run: Laplace
	./Laplace

clean:
	rm -rf *.obj Laplace
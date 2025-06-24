# Makefile to compile and run client and server.
# compile: make
# run server: make runS
# run client: make runC ARG="localhost 6000"
# client arguments are 'localhost' and '6000'
# clean: make clean

# Output targets are binary files 'Client' and 'Server'
Client Server: Client.obj Server.obj
	g++ Client.obj -pthread -o Client
	g++ Server.obj -pthread -o Server

# Intermediate object files.
Client.obj: Client.cpp
	g++ -c Client.cpp -pthread -o Client.obj
Server.obj: Server.cpp
	g++ -c Server.cpp -pthread -o Server.obj

runS: Server
	./Server $(ARG)
runC: Client
	./Client $(ARG)

# Cleanup temporary files.
clean:
	rm -rf *.obj Client Server

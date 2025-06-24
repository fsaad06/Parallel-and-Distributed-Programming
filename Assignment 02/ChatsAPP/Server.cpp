// The Server (Server.cpp)

// ***************************************** Header Files ***********************************************
#include <iostream>
#include <cstring>		// strlen()
#include <cstdlib>		// exit()
#include <arpa/inet.h>	// bind(), listen(), accept(), send(), recv()
#include <unistd.h>
#include <pthread.h>

// ****************************************** #Defintions ***********************************************
#define MAXBUFFERSIZE 512		// Maximum default buffersize
#define SERVERPORT 6000			// Server will be listening on this port by default.

// Adresses
struct sockaddr_in ServerAddress;		// Server's Address.
struct sockaddr_in ClientAddress;		// Client's Address.

// File Descriptors.
int ServerSocketFD, ClientSocketFD;

// Server's Buffer.
char Buffer[MAXBUFFERSIZE];
int opt = 1;
socklen_t sin_size;

// Receive Function : Used for  Diff Threads
void *receiveMessages(void *arg) {
    while (true) {
        int NumOfBytesReceived = recv(ClientSocketFD, Buffer, MAXBUFFERSIZE - 1, 0);
        if (NumOfBytesReceived <= 0) {
            std::cout << "Client disconnected." << std::endl;
            close(ClientSocketFD);
            exit(0);
        }
        Buffer[NumOfBytesReceived] = '\0';
        std::cout << "Client: " << Buffer << std::endl;
    }
    return nullptr;
}

// Send Function : Used for  Diff Threads
void *sendMessages(void *arg) {
    char message[MAXBUFFERSIZE];
    while (true) {
        std::cin.getline(message, MAXBUFFERSIZE);
        send(ClientSocketFD, message, strlen(message), 0);
    }
    return nullptr;
}

int main() {

	//Setting Server
    ServerSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    
	setsockopt(ServerSocketFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ServerAddress.sin_family = AF_INET;			// Socekt family.
    ServerAddress.sin_addr.s_addr = INADDR_ANY;		// Setting server IP. INADDR_ANY is the localhost IP.
    ServerAddress.sin_port = htons(SERVERPORT);			// Setting server port.
    memset(&(ServerAddress.sin_zero), '\0', 8);

	// bind()
    bind(ServerSocketFD, (sockaddr *)&ServerAddress, sizeof(ServerAddress));
    
	// listen
	listen(ServerSocketFD, 1);

	// Accept will block and wait for connections to accept.
    sin_size = sizeof(ClientAddress);
    ClientSocketFD = accept(ServerSocketFD, (sockaddr *)&ClientAddress, &sin_size);
    std::cout << "Connected to " << inet_ntoa(ClientAddress.sin_addr) << std::endl;

	// Parallel / Async Implementation
    pthread_t recvThread, sendThread;
    pthread_create(&recvThread, nullptr, receiveMessages, nullptr);
    pthread_create(&sendThread, nullptr, sendMessages, nullptr);

    pthread_join(recvThread, nullptr);
    pthread_join(sendThread, nullptr);

	// Close Connection
    close(ServerSocketFD);
	close(ClientSocketFD);
    return 0;
}

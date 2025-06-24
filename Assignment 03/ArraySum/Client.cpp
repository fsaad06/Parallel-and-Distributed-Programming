// Including Packages
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <omp.h>
#include "Client.h"

#define MAXBUFFERSIZE 65536
// Port 6000 was already in used in my PC 
#define SERVERPORT 6001

// Same Setup Clients commands used as in Assign 02
int setupClient(const char* serverIP) {
    int clientSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFD < 0) {
        std::cerr << "Error creating client socket" << std::endl;
        return -1;
    }
    int yes = 1;
    setsockopt(clientSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);

    struct hostent* he = gethostbyname(serverIP);
    if (he == NULL) {
        std::cerr << "Error resolving hostname: " << serverIP << std::endl;
        close(clientSocketFD);
        return -1;
    }
    memcpy(&serverAddr.sin_addr, he->h_addr_list[0], he->h_length);
    memset(&(serverAddr.sin_zero), '\0', 8);

    std::cout << "Attempting to connect to " << serverIP << ":" << SERVERPORT << "..." << std::endl;
    if (connect(clientSocketFD, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        close(clientSocketFD);
        return -1;
    }
    //std::cout << "Connected to server!" << std::endl;

    return clientSocketFD;
}

// Debugging Statements commented out for True Comparison of Time
long long distributedClient(const char* serverIP, int size, int clientSocketFD) {
    int halfSize;
    ssize_t bytesRecv = recv(clientSocketFD, &halfSize, sizeof(int), 0);
    if (bytesRecv != sizeof(int)) {
        std::cerr << "Error receiving halfSize" << std::endl;
        return -1;
    }
    //std::cout << "Received halfSize: " << halfSize << std::endl;

    long long* arr = new long long[halfSize];
    size_t totalBytes = halfSize * sizeof(long long);
    size_t totalRecv = 0;
    while(totalRecv < totalBytes) {
        size_t chunkSize = std::min(MAXBUFFERSIZE, static_cast<int>(totalBytes - totalRecv));
        bytesRecv = recv(clientSocketFD, (char*)arr + totalRecv, chunkSize, 0);
        if (bytesRecv <= 0) {
            std::cerr << "Error receiving array data" << std::endl;
            delete[] arr;
            return -1;
        }
        totalRecv += bytesRecv;
    }
    //std::cout << "Received " << halfSize << " elements" << std::endl;

    long long sum = 0;
    #pragma omp parallel for reduction(+:sum)
    for(int i = 0; i < halfSize; i++) {
        sum += arr[i];
    }
    //std::cout << "Computed sum: " << sum << std::endl;

    size_t totalSent = 0;
    while(totalSent < sizeof(long long)) {
        ssize_t bytesSent = send(clientSocketFD, (char*)&sum + totalSent, sizeof(long long) - totalSent, 0);
        if (bytesSent <= 0) {
            std::cerr << "Error sending sum" << std::endl;
            break;
        }
        totalSent += bytesSent;
    }

    delete[] arr;
    return sum;
}
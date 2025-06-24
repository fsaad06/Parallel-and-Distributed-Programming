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
    std::cout << "Connected to server!" << std::endl;

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

    long long** A_half = new long long*[halfSize];
    long long** B = new long long*[size];
    long long** C_half = new long long*[halfSize];
    for(int i = 0; i < halfSize; i++) {
        A_half[i] = new long long[size];
        C_half[i] = new long long[size];
    }
    for(int i = 0; i < size; i++) {
        B[i] = new long long[size];
    }

    size_t rowBytes = size * sizeof(long long);
    for(int i = 0; i < halfSize; i++) {
        size_t totalRecv = 0;
        while(totalRecv < rowBytes) {
            size_t chunkSize = std::min(MAXBUFFERSIZE, static_cast<int>(rowBytes - totalRecv));
            bytesRecv = recv(clientSocketFD, (char*)A_half[i] + totalRecv, chunkSize, 0);
            if (bytesRecv <= 0) {
                std::cerr << "Error receiving row " << i << " of A" << std::endl;
                return -1;
            }
            totalRecv += bytesRecv;
        }
    }
    for(int i = 0; i < size; i++) {
        size_t totalRecv = 0;
        while(totalRecv < rowBytes) {
            size_t chunkSize = std::min(MAXBUFFERSIZE, static_cast<int>(rowBytes - totalRecv));
            bytesRecv = recv(clientSocketFD, (char*)B[i] + totalRecv, chunkSize, 0);
            if (bytesRecv <= 0) {
                std::cerr << "Error receiving row " << i << " of B" << std::endl;
                return -1;
            }
            totalRecv += bytesRecv;
        }
    }
    //std::cout << "Received " << halfSize << " rows of A and " << size << " rows of B" << std::endl;

    long long sum = 0;
    #pragma omp parallel for collapse(2) reduction(+:sum)
    for(int i = 0; i < halfSize; i++) {
        for(int j = 0; j < size; j++) {
            C_half[i][j] = 0;
            for(int k = 0; k < size; k++) {
                C_half[i][j] += A_half[i][k] * B[k][j];
            }
            sum += C_half[i][j];
        }
    // Commented Out for Better Time Comparisons
    // std::ofstream outFile("matrixMul_Distri.csv");
    // for (int i = 0; i < size; i++) {
    //   for (int j = 0; j < size; j++) {
    //        outFile << C[i][j];
    //       if (j != size - 1) outFile << ",";
    //    }
     //   outFile << "\n";
    //  }
    // outFile << "\n \n \n ";
    }
   // std::cout << "Computed sum: " << sum << std::endl;

    size_t totalSent = 0;
    while(totalSent < sizeof(long long)) {
        ssize_t bytesSent = send(clientSocketFD, (char*)&sum + totalSent, sizeof(long long) - totalSent, 0);
        if (bytesSent <= 0) {
            std::cerr << "Error sending sum" << std::endl;
            break;
        }
        totalSent += bytesSent;
    }

    // Dynamic De-allocation
    for(int i = 0; i < halfSize; i++) {
        delete[] A_half[i];
        delete[] C_half[i];
    }
    for(int i = 0; i < size; i++) {
        delete[] B[i];
    }
    delete[] A_half;
    delete[] B;
    delete[] C_half;

    return sum;
}
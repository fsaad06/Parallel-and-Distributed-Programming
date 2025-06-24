// Including Packages
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <unistd.h>
#include <omp.h>
#include "Server.h"

#define MAXBUFFERSIZE 65536

// Port 6000 was already in used in my PC 
#define SERVERPORT 6001

// Same Setup Server commands used as in Assign 02
int setupServer() {
    int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFD < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }
    int yes = 1;
    setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVERPORT);
    memset(&(serverAddr.sin_zero), '\0', 8);

    if (bind(serverSocketFD, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket: ";
        perror("");
        close(serverSocketFD);
        return -1;
    }
    if (listen(serverSocketFD, 1) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocketFD);
        return -1;
    }

    std::cout << "Server waiting for client..." << std::endl;
    struct sockaddr_in clientAddr;
    socklen_t sin_size = sizeof(clientAddr);
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientAddr, &sin_size);
    if (clientSocketFD < 0) {
        std::cerr << "Error accepting connection" << std::endl;
        close(serverSocketFD);
        return -1;
    }
    std::cout << "Client connected!" << std::endl;

    close(serverSocketFD);  // Close listening socket, keep client socket open
    return clientSocketFD;
}

// Debugging Statements commented out for True Comparison of Time
long long distributedServer(long long** A, long long** B, long long** C, int size, int clientSocketFD) {
    int halfSize = size / 2;
    ssize_t bytesSent = send(clientSocketFD, &halfSize, sizeof(int), 0);
    if (bytesSent != sizeof(int)) {
        std::cerr << "Error sending halfSize" << std::endl;
        return -1;
    }

    // Chunk-Based Model : Sending A
    size_t rowBytes = size * sizeof(long long);
    for(int i = 0; i < halfSize; i++) {
        size_t totalSent = 0;
        while(totalSent < rowBytes) {
            size_t chunkSize = std::min(MAXBUFFERSIZE, static_cast<int>(rowBytes - totalSent));
            bytesSent = send(clientSocketFD, (char*)A[i] + totalSent, chunkSize, 0);
            if (bytesSent <= 0) {
                std::cerr << "Error sending row " << i << " of A" << std::endl;
                return -1;
            }
            totalSent += bytesSent;
        }
    }
    // Chunk-Based Model : Sending B
    for(int i = 0; i < size; i++) {
        size_t totalSent = 0;
        while(totalSent < rowBytes) {
            size_t chunkSize = std::min(MAXBUFFERSIZE, static_cast<int>(rowBytes - totalSent));
            bytesSent = send(clientSocketFD, (char*)B[i] + totalSent, chunkSize, 0);
            if (bytesSent <= 0) {
                std::cerr << "Error sending row " << i << " of B" << std::endl;
                return -1;
            }
            totalSent += bytesSent;
        }
    }
    //std::cout << "Sent " << halfSize << " rows of A and " << size << " rows of B to client" << std::endl;

    long long localSum = 0;
    #pragma omp parallel for collapse(2) reduction(+:localSum)
    for(int i = halfSize; i < size; i++) {
        for(int j = 0; j < size; j++) {
            C[i][j] = 0;
            for(int k = 0; k < size; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
            localSum += C[i][j];
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

    //std::cout << "Server local sum: " << localSum << std::endl;

    long long clientSum;
    size_t totalRecv = 0;
    while(totalRecv < sizeof(long long)) {
        ssize_t bytesRecv = recv(clientSocketFD, (char*)&clientSum + totalRecv, sizeof(long long) - totalRecv, 0);
        if (bytesRecv <= 0) {
            std::cerr << "Error receiving client sum, received " << totalRecv << " bytes" << std::endl;
            return -1;
        }
        totalRecv += bytesRecv;
    }
    //std::cout << "Received client sum: " << clientSum << std::endl;

    return localSum + clientSum;
}
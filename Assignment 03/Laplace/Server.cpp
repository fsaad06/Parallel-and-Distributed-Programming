// Including Packages
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <arpa/inet.h>
#include <unistd.h>
#include <omp.h>
#include "Server.h"

#define XSIZE 64
#define YSIZE 64
#define ITER 1000
#define MAXBUFFERSIZE 65536

// Port 6000 was already in used in my PC 
#define SERVERPORT 6001

extern double u[XSIZE][YSIZE], uu[XSIZE][YSIZE];

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

    close(serverSocketFD);
    return clientSocketFD;
}

// Debugging Statements commented out for True Comparison of Time
double distributedServer(int numThreads, int clientSocketFD) {
    omp_set_num_threads(numThreads);
    int halfRows = XSIZE / 2;
    ssize_t bytesSent = send(clientSocketFD, &halfRows, sizeof(int), 0);
    if (bytesSent != sizeof(int)) {
        std::cerr << "Error sending halfRows" << std::endl;
        return -1.0;
    }

    // Chunk-based Model
    size_t totalBytes = XSIZE * YSIZE * sizeof(double);
    size_t totalSent = 0;
    while (totalSent < totalBytes) {
        size_t chunkSize = std::min(static_cast<size_t>(MAXBUFFERSIZE), totalBytes - totalSent);
        bytesSent = send(clientSocketFD, (char*)u + totalSent, chunkSize, 0);
        if (bytesSent <= 0) {
            std::cerr << "Error sending u array at offset " << totalSent << std::endl;
            return -1.0;
        }
        totalSent += bytesSent;
    }
    //std::cout << "Sent u array (" << totalBytes << " bytes) to client" << std::endl;

    double maxDiff = 0.0;
    for (int iter = 0; iter < ITER; iter++) {
        #pragma omp parallel for collapse(2)
        for (int x = 0; x < XSIZE; x++) {
            for (int y = 0; y < YSIZE; y++) {
                uu[x][y] = u[x][y];
            }
        }

        // Server computes rows halfRows to XSIZE-2 (excluding boundary)
        double localMaxDiff = 0.0;
        #pragma omp parallel for collapse(2) reduction(max:localMaxDiff)
        for (int x = halfRows; x < XSIZE - 1; x++) {
            for (int y = 1; y < YSIZE - 1; y++) {
                double newVal = 0.25 * (uu[x-1][y] + uu[x+1][y] + uu[x][y-1] + uu[x][y+1]);
                double diff = std::abs(newVal - u[x][y]);
                if (diff > localMaxDiff) localMaxDiff = diff;
                u[x][y] = newVal;
            }
        }
        maxDiff = localMaxDiff;

        // Exchange rows 1 to halfRows-1 with client
        size_t exchangeBytes = (halfRows - 1) * YSIZE * sizeof(double);
        totalSent = 0;
        while (totalSent < exchangeBytes) {
            size_t chunkSize = std::min(static_cast<size_t>(MAXBUFFERSIZE), exchangeBytes - totalSent);
            bytesSent = send(clientSocketFD, (char*)&u[1][0] + totalSent, chunkSize, 0);
            if (bytesSent <= 0) {
                std::cerr << "Error sending u update at iter " << iter << ", offset " << totalSent << std::endl;
                return -1.0;
            }
            totalSent += bytesSent;
        }

        size_t totalRecv = 0;
        while (totalRecv < exchangeBytes) {
            size_t chunkSize = std::min(static_cast<size_t>(MAXBUFFERSIZE), exchangeBytes - totalRecv);
            ssize_t bytesRecv = recv(clientSocketFD, (char*)&u[1][0] + totalRecv, chunkSize, 0);
            if (bytesRecv <= 0) {
                std::cerr << "Error receiving u update at iter " << iter << ", offset " << totalRecv << std::endl;
                return -1.0;
            }
            totalRecv += bytesRecv;
        }
    }

    // Receive client's maxDiff
    double clientMaxDiff = 0.0;
    size_t totalRecv = 0;
    while (totalRecv < sizeof(double)) {
        ssize_t bytesRecv = recv(clientSocketFD, (char*)&clientMaxDiff + totalRecv, sizeof(double) - totalRecv, 0);
        if (bytesRecv <= 0) {
            std::cerr << "Error receiving client maxDiff at offset " << totalRecv << std::endl;
            return -1.0;
        }
        totalRecv += bytesRecv;
    }
   // std::cout << "Server maxDiff: " << maxDiff << ", Client maxDiff: " << clientMaxDiff << std::endl;

    return std::max(maxDiff, clientMaxDiff);
}
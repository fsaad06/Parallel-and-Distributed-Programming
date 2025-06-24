// Including Packages
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <omp.h>
#include "Client.h"

#define XSIZE 64
#define YSIZE 64
#define ITER 1000
#define MAXBUFFERSIZE 65536
// Port 6000 was already in used in my PC 
#define SERVERPORT 6001

extern double u[XSIZE][YSIZE], uu[XSIZE][YSIZE];

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
double distributedClient(const char* serverIP, int numThreads, int clientSocketFD) {
    omp_set_num_threads(numThreads);

    // Chunk-based Model
    int halfRows;
    ssize_t bytesRecv = recv(clientSocketFD, &halfRows, sizeof(int), 0);
    if (bytesRecv != sizeof(int)) {
        std::cerr << "Error receiving halfRows, received " << bytesRecv << " bytes" << std::endl;
        return -1.0;
    }
    //std::cout << "Received halfRows: " << halfRows << std::endl;

    // Receive full u array
    size_t totalBytes = XSIZE * YSIZE * sizeof(double);
    size_t totalRecv = 0;
    while (totalRecv < totalBytes) {
        size_t chunkSize = std::min(static_cast<size_t>(MAXBUFFERSIZE), totalBytes - totalRecv);
        bytesRecv = recv(clientSocketFD, (char*)u + totalRecv, chunkSize, 0);
        if (bytesRecv <= 0) {
            std::cerr << "Error receiving u array at offset " << totalRecv << ", received " << bytesRecv << " bytes" << std::endl;
            return -1.0;
        }
        totalRecv += bytesRecv;
    }
    //std::cout << "Received u array (" << totalBytes << " bytes)" << std::endl;

    double maxDiff = 0.0;
    for (int iter = 0; iter < ITER; iter++) {
        // Copy u to uu
        #pragma omp parallel for collapse(2)
        for (int x = 0; x < XSIZE; x++) {
            for (int y = 0; y < YSIZE; y++) {
                uu[x][y] = u[x][y];
            }
        }

        // Client computes rows 1 to halfRows-1
        double localMaxDiff = 0.0;
        #pragma omp parallel for collapse(2) reduction(max:localMaxDiff)
        for (int x = 1; x < halfRows; x++) {
            for (int y = 1; y < YSIZE - 1; y++) {
                double newVal = 0.25 * (uu[x-1][y] + uu[x+1][y] + uu[x][y-1] + uu[x][y+1]);
                double diff = std::abs(newVal - u[x][y]);
                if (diff > localMaxDiff) localMaxDiff = diff;
                u[x][y] = newVal;
            }
        }
        maxDiff = localMaxDiff;

        // Exchange rows 1 to halfRows-1 with server
        size_t exchangeBytes = (halfRows - 1) * YSIZE * sizeof(double);
        size_t totalSent = 0;
        while (totalSent < exchangeBytes) {
            size_t chunkSize = std::min(static_cast<size_t>(MAXBUFFERSIZE), exchangeBytes - totalSent);
            ssize_t bytesSent = send(clientSocketFD, (char*)&u[1][0] + totalSent, chunkSize, 0);
            if (bytesSent <= 0) {
                std::cerr << "Error sending u update at iter " << iter << ", offset " << totalSent << std::endl;
                return -1.0;
            }
            totalSent += bytesSent;
        }

        totalRecv = 0;
        while (totalRecv < exchangeBytes) {
            size_t chunkSize = std::min(static_cast<size_t>(MAXBUFFERSIZE), exchangeBytes - totalRecv);
            bytesRecv = recv(clientSocketFD, (char*)&u[1][0] + totalRecv, chunkSize, 0);
            if (bytesRecv <= 0) {
                std::cerr << "Error receiving u update at iter " << iter << ", offset " << totalRecv << std::endl;
                return -1.0;
            }
            totalRecv += bytesRecv;
        }
    }

    // Send maxDiff to server
    size_t totalSent = 0;
    while (totalSent < sizeof(double)) {
        ssize_t bytesSent = send(clientSocketFD, (char*)&maxDiff + totalSent, sizeof(double) - totalSent, 0);
        if (bytesSent <= 0) {
            std::cerr << "Error sending maxDiff at offset " << totalSent << std::endl;
            return -1.0;
        }
        totalSent += bytesSent;
    }

    return maxDiff;
}
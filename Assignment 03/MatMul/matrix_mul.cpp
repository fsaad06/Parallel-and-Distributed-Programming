// Including Packages
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <chrono>
#include <omp.h>
#include <unistd.h>  
#include <iomanip>
#include <fstream>

// Separate Header and Client Files
#include "Server.h"
#include "Client.h"

// Function to allocate a matrix
long long** allocate2DMatrix(int size) {
    long long** mat = new long long*[size];
    for(int i = 0; i < size; i++) {
        mat[i] = new long long[size];
    }
    return mat;
}

void deallocate2DMatrix(long long** mat, int size) {
    for(int i = 0; i < size; i++) {
        delete[] mat[i];
    }
    delete[] mat;
}

void generateMatrix(long long** mat, int size) {
    srand(42);  // Fixed seed for consistency
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            mat[i][j] = rand() % 100+ 1;  // Values 1-100 for simplicity
        }
    }
}

long long serialMatrixMult(long long** A, long long** B, long long** C, int size) {
    long long sum = 0;
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            C[i][j] = 0;
            for(int k = 0; k < size; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
            sum += C[i][j];
        }
    }
   
    // Commented Out for Better Time Comparisons

    // std::ofstream outFile("matrixMul_Serial.csv");
    // for (int i = 0; i < size; i++) {
    //   for (int j = 0; j < size; j++) {
    //        outFile << C[i][j];
    //       if (j != size - 1) outFile << ",";
    //    }
     //   outFile << "\n";
    //  }
    // outFile << "\n \n \n ";
    return sum;
}

long long openMPMatrixMult(long long** A, long long** B, long long** C, int size) {
    long long sum = 0;
    #pragma omp parallel for collapse(2) reduction(+:sum)
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            C[i][j] = 0;
            for(int k = 0; k < size; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
            sum += C[i][j];
        }
    }

    // Commented Out for Better Time Comparisons

    // std::ofstream outFile("matrixMul_OpenMP.csv");
    // for (int i = 0; i < size; i++) {
    //   for (int j = 0; j < size; j++) {
    //        outFile << C[i][j];
    //       if (j != size - 1) outFile << ",";
    //    }
     //   outFile << "\n";
    //  }
    // outFile << "\n \n \n ";
    return sum;
}

int main(int argc, char* argv[]) {
    int sizes[] = {10, 100, 200, 500, 700, 1000};  // Matrix sizes to test
    int numSizes = 6;

    int choice;
    std::cout << "Select implementation mode:\n";
    std::cout << "1. Serial\n";
    std::cout << "2. OpenMP\n";
    std::cout << "3. Distributed (Server/Client)\n";
    std::cout << "Enter choice (1-3): ";
    std::cin >> choice;

    // Variables for distributed mode
    char role = '\0';
    std::string serverIP;
    int clientSocketFD = -1;

    if (choice == 3) {
        std::cout << "Run as (S)erver or (C)lient? ";
        std::cin >> role;
        if (role == 'C' || role == 'c') {
            std::cout << "Enter server IP: ";
            std::cin >> serverIP;
            clientSocketFD = setupClient(serverIP.c_str());
            if (clientSocketFD < 0) {
                std::cerr << "Failed to setup client" << std::endl;
                return -1;
            }
        }
        else if (role == 'S' || role == 's') {
            clientSocketFD = setupServer();
            if (clientSocketFD < 0) {
                std::cerr << "Failed to setup server" << std::endl;
                return -1;
            }
        }
    }

    for (int s = 0; s < numSizes; s++) {
        int N = sizes[s];
        std::cout << "\n"
                  << std::left << std::setw(20) << "Matrix Size"
                  << std::setw(20) << (choice == 3 && (role == 'C' || role == 'c') ? "Client Result" : "Result")
                  << std::setw(20) << "Time Taken (s)" << std::endl;

        // Allocate matrices
        long long** A = allocate2DMatrix(N);
        long long** B = allocate2DMatrix(N);
        long long** C = allocate2DMatrix(N);

        generateMatrix(A, N);
        generateMatrix(B, N);

        if (choice == 1) {
            auto startTime = std::chrono::high_resolution_clock::now();
            long long result = serialMatrixMult(A, B, C, N);
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = endTime - startTime;

            std::cout << std::left << std::setw(20) << (std::to_string(N) + " x " + std::to_string(N))
                      << std::setw(20) << result
                      << std::setw(20) << duration.count()
                      << std::endl;
        }
        else if (choice == 2) {
            auto startTime = std::chrono::high_resolution_clock::now();
            long long result = openMPMatrixMult(A, B, C, N);
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = endTime - startTime;

            std::cout << std::left << std::setw(20) << (std::to_string(N) + " x " + std::to_string(N))
                      << std::setw(20) << result
                      << std::setw(20) << duration.count()
                      << std::endl;
        }
        else if (choice == 3) {
            if (role == 'S' || role == 's') {
                auto startTime = std::chrono::high_resolution_clock::now();
                long long result = distributedServer(A, B, C, N, clientSocketFD);
                auto endTime = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duration = endTime - startTime;
                std::cout << std::left << std::setw(20) << (std::to_string(N) + " x " + std::to_string(N))
                          << std::setw(20) << result
                          << std::setw(20) << duration.count()
                          << std::endl;
            }
            else if (role == 'C' || role == 'c') {
                // Checking Time on Server Side Only for Better 
                long long result = distributedClient(serverIP.c_str(), N, clientSocketFD);
            }
            else {
                std::cout << "Invalid role choice" << std::endl;
                break;
            }
        }
        else {
            std::cout << "Invalid choice" << std::endl;
            break;
        }

        // Deallocate matrices
        deallocate2DMatrix(A, N);
        deallocate2DMatrix(B, N);
        deallocate2DMatrix(C, N);
    }

    // Making Sure Across Machines Distributions
    if (choice == 3 && clientSocketFD != -1) {
        close(clientSocketFD);
    }

    return 0;
}
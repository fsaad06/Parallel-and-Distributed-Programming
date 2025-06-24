// Including Packages
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <chrono>
#include <omp.h>
#include <iomanip>
#include <unistd.h> 

// Separate Header and Client Files
#include "Server.h"
#include "Client.h"

// Dynamic 1D array
long long* allocate1DArray(int size) {
    return new long long[size];
}

void deallocate1DArray(long long* arr) {
    delete[] arr;
}

void generateArray(long long* arr, int size) {
    srand(42);  // Fixed seed for consistency
    for(int i = 0; i < size; i++) {
        arr[i] = rand() % 100 + 1;  // Values 1-100 for simplicity
    }
}

// Part 1 : Serial Implementation
long long serialSum(long long* arr, int size) {
    long long sum = 0;
    for(int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

// Part 2 : OpenMP Implementation
long long openMPSum(long long* arr, int size) {
    long long sum = 0;
    #pragma omp parallel for reduction(+:sum)
    for(int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

int main(int argc, char* argv[]) {

    // Sizes of Tests
    int sizes[] = {10000, 20000, 50000, 70000, 100000, 200000, 1000000, 2000000,10000000, 20000000};  // Array sizes to test
    int numSizes = 10;

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
                  << std::left << std::setw(20) << "Array Size"
                  << std::setw(20) << (choice == 3 && (role == 'C' || role == 'c') ? "Client Result" : "Result")
                  << std::setw(20) << "Time Taken (s)" << std::endl;

        // Allocate array
        long long* arr = allocate1DArray(N);
        generateArray(arr, N);

        if (choice == 1) {
            auto startTime = std::chrono::high_resolution_clock::now();
            long long result = serialSum(arr, N);
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = endTime - startTime;

            std::cout << std::left << std::setw(20) << N
                      << std::setw(20) << result
                      << std::setw(20) << duration.count()
                      << std::endl;
        }
        else if (choice == 2) {
            auto startTime = std::chrono::high_resolution_clock::now();
            long long result = openMPSum(arr, N);
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = endTime - startTime;

            std::cout << std::left << std::setw(20) << N
                      << std::setw(20) << result
                      << std::setw(20) << duration.count()
                      << std::endl;
        }
        else if (choice == 3) {
            if (role == 'S' || role == 's') {
                auto startTime = std::chrono::high_resolution_clock::now();
                long long result = distributedServer(arr, N, clientSocketFD);
                auto endTime = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duration = endTime - startTime;
                std::cout << std::left << std::setw(20) << N
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

        // Deallocate array
        deallocate1DArray(arr);
    }

    // Making Sure Across Machines Distributions
    if (choice == 3 && clientSocketFD != -1) {
        close(clientSocketFD);
    }

    return 0;
}
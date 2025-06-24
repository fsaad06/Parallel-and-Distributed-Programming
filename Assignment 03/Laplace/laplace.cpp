// Including Packages
#include <iostream>
#include <chrono>
#include <omp.h>
#include <iomanip>
#include <unistd.h>
#include <cmath>
#include <Python.h>
#include "matplotlibcpp.h"

// Separate Header and Client Files
#include "Server.h"
#include "Client.h"

#define XSIZE 64
#define YSIZE 64
#define ITER 10000

// 2D arrays for computation
double u[XSIZE][YSIZE], uu[XSIZE][YSIZE];

// Initialization with boundary conditions
void initializeGrid() {
    for (int x = 0; x < XSIZE; x++) {
        for (int y = 0; y < YSIZE; y++) {
            if (x == 0) u[x][y] = 5.0;             // Top boundary
            else if (x == XSIZE-1) u[x][y] = -5.0;  // Bottom boundary
            else if (y == 0 || y == YSIZE-1) u[x][y] = 0.0; // Left/Right
            else u[x][y] = 0.0;                     // Inside grid
        }
    }
}

// Taken from Online Forum
void plotHeatmap() {
    // Flatten the 2D array into a 1D array
    float image[XSIZE * YSIZE];
    for (int x = 0; x < XSIZE; ++x)
        for (int y = 0; y < YSIZE; ++y)
            image[x * YSIZE + y] = static_cast<float>(u[x][y]);
    Py_Initialize();

    // Pass the image data to Python and plot using raw Matplotlib commands
    FILE* fp = fopen("plot.py", "w");
    fprintf(fp, "import numpy as np\n");
    fprintf(fp, "import matplotlib.pyplot as plt\n");
    fprintf(fp, "data = np.array([");
    for (int i = 0; i < XSIZE * YSIZE; ++i) {
        fprintf(fp, "%f", image[i]);
        if (i < XSIZE * YSIZE - 1) fprintf(fp, ",");
    }
    fprintf(fp, "]).reshape(%d, %d)\n", XSIZE, YSIZE);
    fprintf(fp, "plt.imshow(data)\n");
    fprintf(fp, "plt.colorbar()\n");
    fprintf(fp, "plt.title('Laplace Solution')\n");
    fprintf(fp, "plt.show()\n");
    fclose(fp);

    // Execute the Python script
    FILE* pyfile = fopen("plot.py", "r");
    PyRun_SimpleFile(pyfile, "plot.py");
    fclose(pyfile);

    Py_Finalize();
}

// Serial Laplace solver
double serialLaplace() {
    double maxDiff = 0.0;
    for (int iter = 0; iter < ITER; iter++) {
        // Copy u to uu
        for (int x = 0; x < XSIZE; x++) {
            for (int y = 0; y < YSIZE; y++) {
                uu[x][y] = u[x][y];
            }
        }
        // Update u
        maxDiff = 0.0;
        for (int x = 1; x < XSIZE - 1; x++) {
            for (int y = 1; y < YSIZE - 1; y++) {
                double newVal = 0.25 * (uu[x-1][y] + uu[x+1][y] + uu[x][y-1] + uu[x][y+1]);
                double diff = std::abs(newVal - u[x][y]);
                if (diff > maxDiff) maxDiff = diff;
                u[x][y] = newVal;
            }
        }
    }
    return maxDiff;
}

// OpenMP Laplace solver
double openMPLaplace(int numThreads) {
    omp_set_num_threads(numThreads);
    double maxDiff = 0.0;
    for (int iter = 0; iter < ITER; iter++) {
        // Copy u to uu
        #pragma omp parallel for collapse(2)
        for (int x = 0; x < XSIZE; x++) {
            for (int y = 0; y < YSIZE; y++) {
                uu[x][y] = u[x][y];
            }
        }
        // Update u
        double localMaxDiff = 0.0;
        #pragma omp parallel for collapse(2) reduction(max:localMaxDiff)
        for (int x = 1; x < XSIZE - 1; x++) {
            for (int y = 1; y < YSIZE - 1; y++) {
                double newVal = 0.25 * (uu[x-1][y] + uu[x+1][y] + uu[x][y-1] + uu[x][y+1]);
                double diff = std::abs(newVal - u[x][y]);
                if (diff > localMaxDiff) localMaxDiff = diff;
                u[x][y] = newVal;
            }
        }
        maxDiff = localMaxDiff;
    }
    return maxDiff;
}

int main(int argc, char* argv[]) {
    int threadCounts[] = {1, 2, 4, 8, 16};  // Thread counts to test
    int numTests = 5;

    int choice;
    std::cout << "Select execution mode:\n";
    std::cout << "1. Serial\n";
    std::cout << "2. OpenMP\n";
    std::cout << "3. Distributed (Server/Client)\n";
    std::cout << "Enter choice (1-3): ";
    std::cin >> choice;

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

    for (int t = 0; t < numTests; t++) {
        int numThreads = threadCounts[t];
        std::cout << "\n"
                  << std::left << std::setw(20) << "Threads"
                  << std::setw(20) << (choice == 3 && (role == 'C' || role == 'c') ? "Client MaxDiff" : "MaxDiff")
                  << std::setw(20) << "Time Taken (s)" << std::endl;

        initializeGrid();

        if (choice == 1) {
            auto startTime = std::chrono::high_resolution_clock::now();
            double maxDiff = serialLaplace();
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = endTime - startTime;

            std::cout << std::left << std::setw(20) << numThreads
                      << std::setw(20) << maxDiff
                      << std::setw(20) << duration.count()
                      << std::endl;
        }
        else if (choice == 2) {
            auto startTime = std::chrono::high_resolution_clock::now();
            double maxDiff = openMPLaplace(numThreads);
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = endTime - startTime;

            std::cout << std::left << std::setw(20) << numThreads
                      << std::setw(20) << maxDiff
                      << std::setw(20) << duration.count()
                      << std::endl;
        }
        else if (choice == 3) {
            if (role == 'S' || role == 's') {
                auto startTime = std::chrono::high_resolution_clock::now();
                double maxDiff = distributedServer(numThreads, clientSocketFD);
                auto endTime = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duration = endTime - startTime;
                std::cout << std::left << std::setw(20) << numThreads
                          << std::setw(20) << maxDiff
                          << std::setw(20) << duration.count()
                          << std::endl;
            }
            else if (role == 'C' || role == 'c') {
                
                double maxDiff = distributedClient(serverIP.c_str(), numThreads, clientSocketFD);
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
    }

    if (choice == 3 && clientSocketFD != -1) {
        close(clientSocketFD);
    }

    // Print small portion of final grid

    if (!(choice ==3 &&( role == 'C' || role == 'c'))){
    std::cout << "\nFinal grid sample (10x10):\n";
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            std::cout << u[x][y] << " ";
        }
        std::cout << std::endl;
    }

    plotHeatmap() ;
}

    return 0;
}
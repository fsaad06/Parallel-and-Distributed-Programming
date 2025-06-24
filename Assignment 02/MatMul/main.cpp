// Including Necessary Packages
#include <iostream>
#include <pthread.h>
#include <chrono>
#include <cstdlib>

using namespace std;

#define MAX_THREADS 8
#define FIXED_THREADS 4

// Matrix size (NxN) : Square Matrix
int N; 
double** A;
double** B;
double** C;

// Used Same Concept we did in Lab 03 Last Task
struct ThreadArgs {
    int startRow, endRow;
};

// Matrix Multiplication
void* matMul(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;    //  De-Referenced
    for (int i = args->startRow; i < args->endRow; i++) {
        for (int j = 0; j < N; j++) {
            C[i][j] = 0;
            for (int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

/*  Using Dynamic Memory  
Reason: In my virtual environment static memory slowed on increasing matrix size 
above 1000, sometime it even dumps */
void initializeMatrices(int size) {
    N = size;
    A = new double*[N];
    B = new double*[N];
    C = new double*[N];
    for (int i = 0; i < N; i++) {
        A[i] = new double[N];
        B[i] = new double[N];
        C[i] = new double[N];

        // Setting Elements to randomly in range 0-9
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 10;
            B[i][j] = rand() % 10;
        }
    }
}

// Memory Deallocation
void deleteMatrices() {
    for (int i = 0; i < N; i++) {
        delete[] A[i];
        delete[] B[i];
        delete[] C[i];
    }
    delete[] A;
    delete[] B;
    delete[] C;
}

void runWithThreads(int numThreads) {
    pthread_t threads[numThreads];
    ThreadArgs args[numThreads];
    int rowsPerThread = N / numThreads;

    auto start_time = chrono::high_resolution_clock::now();

    for (int i = 0; i < numThreads; i++) {
        args[i].startRow = i * rowsPerThread;

        // Logic Discussed in Lab
        args[i].endRow = (i == numThreads - 1) ? N : (i + 1) * rowsPerThread;
        pthread_create(&threads[i], NULL, matMul, &args[i]);
    }
    
    // Waiting For Threads
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> execution_time = end_time - start_time;
    cout << "Threads: " << numThreads << " | Execution Time: " << execution_time.count() << " sec" << endl;
}

void fixedThreadScaling() {
    int sizes[] = {100, 200, 400, 600, 800, 1200, 1600, 2400, 3200};
    cout << "\nScaling with Fixed 4 Threads:\n";
    for (int size : sizes) {
        cout << " >> Matrix Size : " << size << endl;
        initializeMatrices(size);
        runWithThreads(FIXED_THREADS);
        deleteMatrices();
    }
}

int main() {
    // ---------------- Part 01 :Thread scaling test ----------------
    cout << "\nThread Scaling Test:\n";
    N = 2000; // Fixed matrix size for thread scaling
    initializeMatrices(N);
    for (int threads = 1; threads <= MAX_THREADS; threads++) {
        runWithThreads(threads);
    }
    deleteMatrices();

    // ---- Part 02 :Matrix size scaling test with fixed 4 threads ----
    fixedThreadScaling();
    
    return 0;
}

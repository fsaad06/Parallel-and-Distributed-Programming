#include <iostream>
#include <omp.h>
#include <iomanip>
#include <cmath>
#include <vector>
#include <mpi.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>

#define MAX_SIZE 1024
#define MIN_SIZE 64
#define ITER 10000
#define SMALL_ITER 1000

// Timing struct
struct msClock {
    typedef std::chrono::high_resolution_clock clock;
    std::chrono::time_point<clock> t1, t2;
    void Start() { t1 = clock::now(); }
    void Stop() { t2 = clock::now(); }
    double ElapsedTime() {
        std::chrono::duration<double, std::milli> ms_doubleC = t2 - t1;
        return ms_doubleC.count();
    }
} Clock;

// Initialize grid with boundary conditions
void initializeGrid(double* grid, int xsize, int ysize) {
    for (int x = 0; x < xsize; x++) {
        for (int y = 0; y < ysize; y++) {
            int idx = x * ysize + y;
            if (x == 0) grid[idx] = 5.0;             // Top boundary
            else if (x == xsize-1) grid[idx] = -5.0;  // Bottom boundary
            else if (y == 0 || y == ysize-1) grid[idx] = 0.0; // Left/Right
            else grid[idx] = 0.0;                     // Inside grid
        }
    }
}

// Compute difference between sums of two matrices
double diffMat(double* M1, double* M2, int rows, int cols) {
    double sum1 = 0.0, sum2 = 0.0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            sum1 += M1[j + i * cols];
            sum2 += M2[j + i * cols];
        }
    }
    return std::abs(sum2 - sum1);
}

// Serial Laplace solver
void serialLaplace(double* u, double* uu, int xsize, int ysize, int iter) {
    for (int i = 0; i < iter; i++) {
        // Copy u to uu
        for (int x = 0; x < xsize; x++) {
            for (int y = 0; y < ysize; y++) {
                uu[x * ysize + y] = u[x * ysize + y];
            }
        }
        // Update u
        for (int x = 1; x < xsize - 1; x++) {
            for (int y = 1; y < ysize - 1; y++) {
                u[x * ysize + y] = 0.25 * (uu[(x-1) * ysize + y] + uu[(x+1) * ysize + y] +
                                           uu[x * ysize + (y-1)] + uu[x * ysize + (y+1)]);
            }
        }
    }
}

// OpenMP Laplace solver
double openMPLaplace(double* u, double* uu, int xsize, int ysize, int iter, int numThreads, double* serial_u) {
    omp_set_num_threads(numThreads);
    for (int i = 0; i < iter; i++) {
        // Copy u to uu
        #pragma omp parallel for
        for (int x = 0; x < xsize; x++) {
            for (int y = 0; y < ysize; y++) {
                uu[x * ysize + y] = u[x * ysize + y];
            }
        }
        // Update u
        #pragma omp parallel for
        for (int x = 1; x < xsize - 1; x++) {
            for (int y = 1; y < ysize - 1; y++) {
                u[x * ysize + y] = 0.25 * (uu[(x-1) * ysize + y] + uu[(x+1) * ysize + y] +
                                           uu[x * ysize + (y-1)] + uu[x * ysize + (y+1)]);
            }
        }
    }
    return diffMat(u, serial_u, xsize, ysize);
}

// MPI Laplace solver
double mpiLaplace(double* local_u, double* local_uu, int local_rows, int global_xsize, int ysize, int iter, int rank, int size, double* serial_u) {
    int rows_per_proc = global_xsize / size;
    int remainder = global_xsize % size;
    std::vector<int> counts(size), displs(size);
    int offset = 0;
    for (int i = 0; i < size; i++) {
        int rows = rows_per_proc + (i < remainder ? 1 : 0);
        counts[i] = rows * ysize;
        displs[i] = offset;
        offset += counts[i];
    }

    double* upper_halo = new double[ysize];
    double* lower_halo = new double[ysize];

    for (int i = 0; i < iter; i++) {
        // Copy local_u to local_uu
        for (int x = 0; x < local_rows; x++) {
            for (int y = 0; y < ysize; y++) {
                local_uu[x * ysize + y] = local_u[x * ysize + y];
            }
        }

        // Exchange halo rows
        int upper_neighbor = (rank == 0) ? MPI_PROC_NULL : rank - 1;
        int lower_neighbor = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

        MPI_Sendrecv(local_u + (local_rows - 1) * ysize, ysize, MPI_DOUBLE, lower_neighbor, 0,
                     upper_halo, ysize, MPI_DOUBLE, upper_neighbor, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(local_u, ysize, MPI_DOUBLE, upper_neighbor, 1,
                     lower_halo, ysize, MPI_DOUBLE, lower_neighbor, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Update local_u
        for (int x = 1; x < local_rows - 1; x++) {
            for (int y = 1; y < ysize - 1; y++) {
                local_u[x * ysize + y] = 0.25 * (local_uu[(x-1) * ysize + y] + local_uu[(x+1) * ysize + y] +
                                                 local_uu[x * ysize + (y-1)] + local_uu[x * ysize + (y+1)]);
            }
        }

        if (rank > 0 && local_rows > 0) { // Upper boundary row
            for (int y = 1; y < ysize - 1; y++) {
                local_u[0 * ysize + y] = 0.25 * (upper_halo[y] + local_uu[1 * ysize + y] +
                                                 local_uu[0 * ysize + (y-1)] + local_uu[0 * ysize + (y+1)]);
            }
        }
        if (rank < size - 1 && local_rows > 0) { // Lower boundary row
            for (int y = 1; y < ysize - 1; y++) {
                local_u[(local_rows-1) * ysize + y] = 0.25 * (local_uu[(local_rows-2) * ysize + y] + lower_halo[y] +
                                                              local_uu[(local_rows-1) * ysize + (y-1)] + local_uu[(local_rows-1) * ysize + (y+1)]);
            }
        }
    }

    // Gather local_u to global_u for comparison
    double* global_u = NULL;
    if (rank == 0) global_u = new double[global_xsize * ysize];
    MPI_Gatherv(local_u, local_rows * ysize, MPI_DOUBLE,
                global_u, &counts[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double diff = 0.0;
    if (rank == 0) {
        diff = diffMat(global_u, serial_u, global_xsize, ysize);
        delete[] global_u;
    }
    MPI_Bcast(&diff, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    delete[] upper_halo;
    delete[] lower_halo;
    return diff;
}

// Save 2D matrix to CSV
void saveMatrixToCSV(double* matrix, int xsize, int ysize, int size, const std::string& filename) {
    std::ofstream file(filename);
    for (int x = 0; x < xsize; x++) {
        for (int y = 0; y < ysize; y++) {
            file << matrix[x * ysize + y];
            if (y < ysize - 1) file << ",";
        }
        file << "\n";
    }
    file.close();
}

// Print 4x4 grid
void printGrid(double* grid, int xsize, int ysize) {
    std::cout << "4x4 Grid:\n+----------------------------+\n";
    for (int x = 0; x < xsize; x++) {
        std::cout << "|";
        for (int y = 0; y < ysize; y++) {
            std::cout << std::fixed << std::setprecision(2) << std::setw(4) << grid[x * ysize + y] << " ";
        }
        std::cout << "|\n";
    }
    std::cout << "+----------------------------+\n";
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Collect node names (for potential debugging, but don't print)
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);
    processor_name[name_len] = '\0';

    char* all_names = NULL;
    if (rank == 0) all_names = new char[size * MPI_MAX_PROCESSOR_NAME];
    MPI_Gather(processor_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
               all_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "MPI on " << size << " process(es):\n\n";
        delete[] all_names;
    }

    // Test 4x4 grid
    const int small_size = 4;
    double* serial_u = NULL;
    double* omp_u = NULL;
    double* mpi_u = NULL;

    if (rank == 0) {
        std::cout << "4x4 Test\n";
        serial_u = new double[small_size * small_size];
        double* serial_uu = new double[small_size * small_size];
        initializeGrid(serial_u, small_size, small_size);
        serialLaplace(serial_u, serial_uu, small_size, small_size, SMALL_ITER);
        std::cout << "Serial:\n";
        printGrid(serial_u, small_size, small_size);

        omp_u = new double[small_size * small_size];
        double* omp_uu = new double[small_size * small_size];
        initializeGrid(omp_u, small_size, small_size);
        double diff_omp = openMPLaplace(omp_u, omp_uu, small_size, small_size, SMALL_ITER, 1, serial_u);
        std::cout << "OpenMP (1 thread) vs Serial Diff: " << std::scientific << std::setprecision(2) << diff_omp << "\n";

        delete[] serial_uu;
        delete[] omp_uu;
    }

    // MPI 4x4 test
    int rows_per_proc = small_size / size;
    int remainder = small_size % size;
    std::vector<int> local_rows_per_rank(size);
    int offset = 0;
    std::vector<int> counts(size), displs(size);
    for (int i = 0; i < size; i++) {
        local_rows_per_rank[i] = rows_per_proc + (i < remainder ? 1 : 0);
        counts[i] = local_rows_per_rank[i] * small_size;
        displs[i] = offset;
        offset += counts[i];
    }
    int local_rows = local_rows_per_rank[rank];

    double* global_u = NULL;
    if (rank == 0) {
        global_u = new double[small_size * small_size];
        initializeGrid(global_u, small_size, small_size);
        mpi_u = new double[small_size * small_size];
    }
    double* local_u = new double[local_rows * small_size];
    double* local_uu = new double[local_rows * small_size];

    // Handle zero rows case
    if (local_rows > 0) {
        initializeGrid(local_u, local_rows, small_size);
    }

    MPI_Scatterv(global_u, &counts[0], &displs[0], MPI_DOUBLE,
                 local_u, local_rows * small_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double diff_mpi = mpiLaplace(local_u, local_uu, local_rows, small_size, small_size, SMALL_ITER, rank, size, serial_u);

    if (rank == 0) {
        std::cout << "MPI (1 process) vs Serial Diff: " << std::scientific << std::setprecision(2) << diff_mpi << "\n\n";
        delete[] serial_u;
        delete[] omp_u;
        delete[] mpi_u;
        delete[] global_u;
    }
    delete[] local_u;
    delete[] local_uu;

    // Performance tests
    std::vector<int> sizes = {64, 128, 256, 512, 1024};
    std::vector<int> thread_counts = {1, 2, 4, 8, 16};
    std::vector<double> serial_times(sizes.size());
    std::vector<std::vector<double>> omp_times(thread_counts.size(), std::vector<double>(sizes.size()));
    std::vector<double> mpi_times(sizes.size());

    for (size_t s = 0; s < sizes.size(); s++) {
        int xsize = sizes[s];
        int ysize = xsize;

        // Serial tests
        if (rank == 0) {
            std::cout << "Serial Tests\n";
            double* u = new double[xsize * ysize];
            double* uu = new double[xsize * ysize];
            initializeGrid(u, xsize, ysize);
            Clock.Start();
            serialLaplace(u, uu, xsize, ysize, ITER);
            Clock.Stop();
            serial_times[s] = Clock.ElapsedTime() / 1000.0;
            std::stringstream ss;
            ss << xsize << "x" << xsize;
            std::cout << std::left << std::setw(8) << "Serial" << "Size " << std::setw(9) << ss.str()
                      << "Time " << std::fixed << std::setprecision(2) << serial_times[s] << "s\n";
            delete[] u;
            delete[] uu;
        }

        // OpenMP tests
        if (rank == 0) {
            std::cout << "OpenMP Tests\n";
            double* u = new double[xsize * ysize];
            double* uu = new double[xsize * ysize];
            double* serial_u = new double[xsize * ysize];
            initializeGrid(u, xsize, ysize);
            initializeGrid(serial_u, xsize, ysize);
            serialLaplace(serial_u, uu, xsize, ysize, ITER);
            for (size_t t = 0; t < thread_counts.size(); t++) {
                initializeGrid(u, xsize, ysize);
                Clock.Start();
                double diff_omp = openMPLaplace(u, uu, xsize, ysize, ITER, thread_counts[t], serial_u);
                Clock.Stop();
                omp_times[t][s] = Clock.ElapsedTime() / 1000.0;
                std::stringstream ss_size, ss_threads;
                ss_size << xsize << "x" << xsize;
                ss_threads << thread_counts[t];
                std::cout << std::left << std::setw(8) << "OpenMP" << "Size " << std::setw(9) << ss_size.str()
                          << "Thr " << std::setw(2) << ss_threads.str()
                          << " Diff " << std::scientific << std::setprecision(2) << diff_omp
                          << " Time " << std::fixed << std::setprecision(2) << omp_times[t][s] << "s\n";
            }
            delete[] u;
            delete[] uu;
            delete[] serial_u;
        }

        // MPI tests
        if (rank == 0) {
            std::cout << "MPI Tests\n";
        }
        if (size >= 1 && size <= 16) {
            int rows_per_proc = xsize / size;
            int remainder = xsize % size;
            int local_rows = rows_per_proc + (rank < remainder ? 1 : 0);
            double* global_u = NULL;
            double* serial_u = NULL;
            if (rank == 0) {
                global_u = new double[xsize * ysize];
                serial_u = new double[xsize * ysize];
                initializeGrid(global_u, xsize, ysize);
                initializeGrid(serial_u, xsize, ysize);
                serialLaplace(serial_u, global_u, xsize, ysize, ITER);
            }

            double* local_u = new double[local_rows * ysize];
            double* local_uu = new double[local_rows * ysize];

            std::vector<int> counts(size), displs(size);
            int offset = 0;
            for (int i = 0; i < size; i++) {
                int rows = rows_per_proc + (i < remainder ? 1 : 0);
                counts[i] = rows * ysize;
                displs[i] = offset;
                offset += counts[i];
            }
            MPI_Scatterv(global_u, &counts[0], &displs[0], MPI_DOUBLE,
                         local_u, local_rows * ysize, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            Clock.Start();
            double diff_mpi = mpiLaplace(local_u, local_uu, local_rows, xsize, ysize, ITER, rank, size, serial_u);
            Clock.Stop();
            double local_time = Clock.ElapsedTime() / 1000.0;

            if (rank == 0) {
                std::stringstream ss;
                ss << "global_u_size_" << xsize << "_procs_" << size << ".csv";
                saveMatrixToCSV(global_u, xsize, ysize, size, ss.str());
            }

            double max_time;
            MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
            if (rank == 0) {
                mpi_times[s] = max_time;
                std::stringstream ss_size, ss_procs;
                ss_size << xsize << "x" << xsize;
                ss_procs << size;
                std::cout << std::left << std::setw(8) << "MPI" << "Size " << std::setw(9) << ss_size.str()
                          << "Proc " << std::setw(2) << ss_procs.str()
                          << " Diff " << std::scientific << std::setprecision(2) << diff_mpi
                          << " Time " << std::fixed << std::setprecision(2) << mpi_times[s] << "s\n";
            }

            if (rank == 0) {
                delete[] global_u;
                delete[] serial_u;
            }
            delete[] local_u;
            delete[] local_uu;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) std::cout << "\n";
    }

    // Performance table
    if (rank == 0) {
        std::cout << "+-----------------------------------------+\n";
        std::cout << "| Performance Table (Times in Seconds)    |\n";
        std::cout << "+-----+-------+-------+-------+-------+-------+\n";
        std::cout << "| Conf| 64x64 |128x128|256x256|512x512|1024x1024|\n";
        std::cout << "+-----+-------+-------+-------+-------+-------+\n";

        // Serial
        std::cout << "| Ser |";
        for (size_t s = 0; s < sizes.size(); s++) {
            std::cout << std::fixed << std::setprecision(2) << std::setw(6) << serial_times[s] << " |";
        }
        std::cout << "\n+-----+-------+-------+-------+-------+-------+\n";

        // OpenMP
        for (size_t t = 0; t < thread_counts.size(); t++) {
            std::stringstream ss;
            ss << thread_counts[t];
            std::cout << "| OMP" << std::setw(2) << ss.str() << "|";
            for (size_t s = 0; s < sizes.size(); s++) {
                std::cout << std::fixed << std::setprecision(2) << std::setw(6) << omp_times[t][s] << " |";
            }
            std::cout << "\n";
        }
        std::cout << "+-----+-------+-------+-------+-------+-------+\n";

        // MPI
        if (size >= 1 && size <= 16) {
            std::stringstream ss;
            ss << size;
            std::cout << "| MPI" << std::setw(2) << ss.str() << "|";
            for (size_t s = 0; s < sizes.size(); s++) {
                std::cout << std::fixed << std::setprecision(2) << std::setw(6) << mpi_times[s] << " |";
            }
            std::cout << "\n";
        }
        std::cout << "+-----+-------+-------+-------+-------+-------+\n";
    }

    MPI_Finalize();
    return 0;
}

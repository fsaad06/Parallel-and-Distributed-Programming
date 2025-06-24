#ifndef SERVER_H
#define SERVER_H

// Variable for Across Different Machines Distribution
int setupServer();

// Function for Chunk-based Server Model
double distributedServer(int numThreads, int clientSocketFD);

#endif
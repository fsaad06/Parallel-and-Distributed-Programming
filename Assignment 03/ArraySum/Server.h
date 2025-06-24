#ifndef SERVER_H
#define SERVER_H

// Variable for Across Different Machines Distribution
int setupServer();

// Function for Chunk-based Server Model
long long distributedServer(long long* arr, int size, int clientSocketFD);

#endif
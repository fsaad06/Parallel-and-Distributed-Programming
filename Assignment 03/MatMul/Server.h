#ifndef SERVER_H
#define SERVER_H

// Variable for Across Different Machines Distribution
int setupServer();

// Function for Chunk-based Server Model
long long distributedServer(long long** A, long long** B, long long** C, int size, int clientSocketFD);

#endif
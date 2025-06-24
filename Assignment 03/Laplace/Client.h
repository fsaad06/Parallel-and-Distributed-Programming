#ifndef CLIENT_H
#define CLIENT_H

// For Cross Machines Distribution 
int setupClient(const char* serverIP);
double distributedClient(const char* serverIP, int numThreads, int clientSocketFD);

#endif
#ifndef CLIENT_H
#define CLIENT_H

// For Cross Machines Distribution 
int setupClient(const char* serverIP);
long long distributedClient(const char* serverIP, int size, int clientSocketFD);

#endif
// The Client (Client.cpp)

// ***************************************** Header Files ***********************************************
#include <iostream>
#include <cstring>		// strlen()
#include <cstdlib>		// exit()
#include <netdb.h>		// gethostbyname(), connect(), send(), recv()
#include <arpa/inet.h> 
#include <unistd.h>
#include <thread>       // Threading


// ****************************************** #Defintions ***********************************************
#define MAXBUFFERSIZE 512   // Maximum default buffersize.
#define SERVERPORT 6000     // Server will be listening on this port 
#define SERVERIP "127.0.0.1"    // Pre-Set IP Address

using namespace std;

// Client's Buffer.
int ClientSocketFD;
char Buffer[MAXBUFFERSIZE];

// Receive Function : Used for  Diff Threads
void receiveMessages() {
    while (true) {
        int numBytes = recv(ClientSocketFD, Buffer, MAXBUFFERSIZE - 1, 0);
        if (numBytes <= 0) break;
        Buffer[numBytes] = '\0';
        cout << "Server: " << Buffer << endl;
    }
}

// Send Function : Used for  Diff Threads
void sendMessages() {
    string message;
    while (true) {
        getline(cin, message);
        send(ClientSocketFD, message.c_str(), message.length(), 0);
    }
}

int main() {
    ClientSocketFD = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in ServerAddress;

    // Initializing Client address for binding.
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(SERVERPORT);
    inet_pton(AF_INET, SERVERIP, &ServerAddress.sin_addr);
    memset(&(ServerAddress.sin_zero), 0, 8);

    connect(ClientSocketFD, (sockaddr *)&ServerAddress, sizeof(ServerAddress));
    cout << "Connected to server" << endl;

    // Async Implementation
    thread receiveThread(receiveMessages);
    thread sendThread(sendMessages);
    
    // Waiting for Threading
    receiveThread.join();
    sendThread.join();
    
    // Close client socket and exit.
    close(ClientSocketFD);
    return 0;
}
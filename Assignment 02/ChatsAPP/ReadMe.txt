Asynchronous Client-Server Application

This program allows a client and server to communicate over a TCP connection using multi-threading.

How to Run:
1. First, compile everything using:
   make

2. Run the server in one terminal:
   make runS

3. Run the client in another terminal: (Default Arguments Already Set)
   make runC

Features:
- Multi-threaded server using pthread.
- Server and client can send messages to each other.
- Default arguments are already set.
- Supports multiple client connections.

Requirements:
- GCC Compiler
- pthread library
- make utility

Troubleshooting:
- Ensure make is installed.
- Make sure port 6000 is available.
- Run the server before starting the client.

For any issues, contact:
Saad Farrukh - saadfarrukh303@gmail.com

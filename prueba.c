#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 7778

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[1024];

    // Create a socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Configure server details
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT); // Make sure it matches the server's port
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server's IP

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error connecting");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Send a message
    strcpy(buffer, "Hello, server!\r\n");
    send(clientSocket, buffer, strlen(buffer), 0);

    // Receive a message
    recv(clientSocket, buffer, sizeof(buffer), 0);
    printf("Server says: %s\n", buffer);

    strcpy(buffer, "hola");
    send(clientSocket, buffer, strlen(buffer), 0);

    // Close the socket
    close(clientSocket);

    return 0;
}

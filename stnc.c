#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_SIZE 104857
void *connection_handler(void *);
int client(int argc, char *argv[]) {
    
    int sock = 0,valread = -1;
    valread++;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE] = {0};
    if(argc != 4){
        printf("\nUsage: %s -c IP PORT\n", argv[0]);
        return -1;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    // Set socket address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));

    // Convert IPv4 and store in sin_addr
    if(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server socket
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\nConnection Failed \n");
        return -1;
    }
    printf("Connection succeed!\n");
    // Start chat loop
    while(1) {
        printf("Write: ");
        // Read input from stdin
        fgets(buffer, BUF_SIZE, stdin);
        // Send input to server socket
        send(sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, BUF_SIZE);
        // Receive data from server socket
        valread = read(sock, buffer, BUF_SIZE);
        // Output received data to stdout
        printf("Answer: %s\n",buffer);
        memset(buffer, 0, BUF_SIZE);
    }
    
    // Close socket
    close(sock);

    return 0;
}
int server(int argc, char *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    if(argc != 3){
        printf("\nUsage: %s -s PORT\n", argv[0]);
        return -1;
    }
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        printf("\nSocket creation error \n");
        return -1;
    }
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | 15, &opt, sizeof(opt))){
        printf("\nSetsockopt error \n");
        return -1;
    }

    memset(&address, '0', sizeof(address));

    // Set socket address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[2]));
    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0){
        printf("\nBind failed \n");
        return -1;
    }
    // Listen for incoming connections
    if (listen(server_fd, 3) < 0){
        printf("\nListen error \n");
        return -1;
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            printf("\nAccept error \n");
    }
    while(1) {
        
        // Spawn new thread to handle communication
        pthread_t sniffer_thread;
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) &new_socket) < 0) {
            printf("\nPthread create error \n");
            return -1;
        }
    }

    return 0;
}
void *connection_handler(void *socket_desc)
{
    // Get connected socket descriptor
    int sock = *(int*)socket_desc;
    int valread;
    char buffer[BUF_SIZE] = {0};

    // Start chat loop
    while(1) {
        // Receive data from client socket
        valread = read(sock, buffer, BUF_SIZE);
        if(valread == 0) {
            break;
        }
        // Output received data to stdout
        printf("Answer: %s\n",buffer);
        // Read input from stdin
        memset(buffer, 0, BUF_SIZE);
        printf("Write: ");
        fgets(buffer, BUF_SIZE, stdin);
        // Send input to client socket
        send(sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, BUF_SIZE);
    }

    // Close socket
    close(sock);

    return 0;
}
int main(int argc, char *argv[])
{

    if (!strcmp(argv[1],"-c"))
    {
        client(argc,argv);
    }
    else if (!strcmp(argv[1],"-s"))
    {
        server(argc,argv);
    }
    else
    {
        printf("Please enter an input in the right format.");
    }
    return 0;
}
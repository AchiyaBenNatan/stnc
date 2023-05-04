#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>

#define BUF_SIZE 104857
int client(int argc, char *argv[]) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    if (inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) <= 0) {
        perror("address invalid or not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    fd_set readfds;
    char message[BUF_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int maxfd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;
        
        select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &readfds)) {
            int n = recv(sockfd, message, BUF_SIZE, 0);
            if (n == -1) {
                perror("error receiving message");
                exit(EXIT_FAILURE);
            } else if (n == 0) {
                printf("server closed the connection\n");
                break;
            } else {
                message[n] = '\0';
                printf("received message from server: %s\n", message);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(message, BUF_SIZE, stdin) == NULL) {
                perror("error reading input");
                exit(EXIT_FAILURE);
            }
            int n = send(sockfd, message, strlen(message), 0);
            if (n == -1) {
                perror("error sending message");
                exit(EXIT_FAILURE);
            } else {
                printf("sent message to server: %s\n", message);
            }
        }
    }
    close(sockfd);
    return 0;
}
int server(int argc, char *argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    fd_set readfds;
    int max_sd, sd;

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Attach socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | 15, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[2]));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %s\n", argv[2]);
    // Initialize the fd_set
    FD_ZERO(&readfds);
    // Add the server socket to the fd_set
    FD_SET(server_fd, &readfds);
    max_sd = server_fd;

    while (1)
    {
        memset(buffer,0,sizeof(buffer));
        // Wait for activity on one of the sockets
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
        // Check if activity is on the server socket
        if (FD_ISSET(server_fd, &readfds))
        {
            // Accept new connection
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("New connection, socket fd is %d, ip is: %s\n", new_socket, inet_ntoa(address.sin_addr));

            // Add the new socket to the fd_set
            FD_SET(new_socket, &readfds);
            if (new_socket > max_sd)
            {
                max_sd = new_socket;
            }
        }

        // Check if activity is on one of the client sockets
        for (sd = server_fd + 1; sd <= max_sd; sd++)
        {
            if (FD_ISSET(sd, &readfds))
            {
                if ((valread = read(sd, buffer, 1024)) == 0)
                {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Host disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    FD_CLR(sd, &readfds);
                }
                else
                {
                    // Handle the client message
                    printf("Received message from client: %s\n", buffer);
                    char message[BUF_SIZE];
                    //fgets(message, BUF_SIZE, stdin);
                    send(sd, "message", strlen("message"), 0);
                    sleep(0.5);
                    send(sd, "message", strlen("message"), 0);
                    sleep(0.5);
                    send(sd, "message", strlen("message"), 0);
                }
            }
        }
    }
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>

#define BUF_SIZE 1024
int client(int argc, char *argv[])
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    if (inet_pton(AF_INET, (const char *)argv[2], &serv_addr.sin_addr) <= 0)
    {
        perror("address invalid or not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("connected to server\n");

    fd_set readfds;
    char message[BUF_SIZE];

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        int maxfd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;
        int res = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (res == -1)
        {
            perror("error in select");
            exit(EXIT_FAILURE);
        }
        else if (res == 0)
        {
            // puts("timeout");
            continue; // no data available from either socket or stdin
        }
        else
        {
            // printf("Select returned for fd: %d\n", res);
            if (FD_ISSET(STDIN_FILENO, &readfds))
            {
                if (fgets(message, BUF_SIZE, stdin) == NULL)
                {
                    perror("error reading input");
                    exit(EXIT_FAILURE);
                }
                int bytes = send(sockfd, message, strlen(message), 0);
                if (bytes == -1)
                {
                    perror("error sending message");
                    exit(EXIT_FAILURE);
                }
                /*
                else
                {
                    printf("sent message to server: %s\n", message);
                }
                */
            }
            if (FD_ISSET(sockfd, &readfds))
            {
                int bytes = recv(sockfd, message, BUF_SIZE, 0);
                if (bytes == -1)
                {
                    perror("error receiving message");
                    exit(EXIT_FAILURE);
                }
                else if (bytes == 0)
                {
                    printf("server closed the connection\n");
                    break;
                }
                else
                {
                    message[bytes] = '\0';
                    printf("Server msg: %s", message);
                }
            }
        }
    }
    close(sockfd);
    return 0;
}
int server(int argc, char *argv[])
{
    int server_fd, clientSocket;
    struct sockaddr_in address;
    int opt = 1;
    char buffer[BUF_SIZE] = {0};
    fd_set readfds;

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

    // Accept and handle incoming connections
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddressLen = sizeof(clientAddress);
    clientSocket = accept(server_fd, (struct sockaddr *)&clientAddress, &clientAddressLen);
    if (clientSocket == -1)
    {
        printf("listen failed with error code : %d", errno);
        close(server_fd);
        close(clientSocket);
        return -1;
    }
    printf("Client connected: %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        int maxfd = (server_fd > clientSocket) ? server_fd : clientSocket;
        int res = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (res == -1)
        {
            perror("error in select");
            exit(EXIT_FAILURE);
        }
        else if (res == 0)
        {
            continue;
        }
        else
        {
            if (FD_ISSET(STDIN_FILENO, &readfds))
            {
                if (fgets(buffer, BUF_SIZE, stdin) == NULL)
                {
                    perror("error reading input");
                    exit(EXIT_FAILURE);
                }
                int bytes = send(clientSocket, buffer, strlen(buffer), 0);
                if (bytes == -1)
                {
                    perror("error sending message");
                    exit(EXIT_FAILURE);
                }
                /*
                else
                {
                    printf("sent message to client: %s\n", message);
                }
                */
            }

            if (FD_ISSET(clientSocket, &readfds))
            {
                int bytes = read(clientSocket, buffer, BUF_SIZE);
                if (bytes == -1)
                {
                    perror("error receiving message");
                    exit(EXIT_FAILURE);
                }
                else if (bytes == 0)
                {
                    // client closed the connection
                    printf("Client disconnected\n");
                    close(clientSocket);
                    FD_CLR(clientSocket, &readfds);
                    clientSocket = accept(server_fd, (struct sockaddr *)&clientAddress, &clientAddressLen);
                    if (clientSocket == -1)
                    {
                        printf("listen failed with error code : %d", errno);
                        close(server_fd);
                        close(clientSocket);
                        return -1;
                    }
                    printf("Client connected: %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
                }
                else
                {
                    buffer[bytes] = '\0';
                    printf("Client msg: %s", buffer);
                }
            }
        }
    }
    return 0;
}
int main(int argc, char *argv[])
{

    if (!strcmp(argv[1], "-c"))
    {
        client(argc, argv);
    }
    else if (!strcmp(argv[1], "-s"))
    {
        server(argc, argv);
    }
    else
    {
        printf("Please enter an input in the right format.\n");
    }
    return 0;
}

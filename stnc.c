#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>

#define BUF_SIZE 104857
int client(int argc, char *argv[])
{

    int sock = 0, valread = -1;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE] = {0};
    fd_set read_fds;

    if (argc != 4)
    {
        printf("\nUsage: %s IP PORT\n", argv[0]);
        return -1;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    // Set socket address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));

    // Convert IPv4 and store in sin_addr
    if (inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server socket
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to server\n");

    // Add socket to read_fds set
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    int flagClient = 0;
    // Wait for activity on socket
    int activity = select(sock + 1, &read_fds, NULL, NULL, NULL);
    while (1)
    {
        if (flagClient == 0)
        {
            read(sock, buffer, BUF_SIZE);
        }
        flagClient = 1;
        memset(buffer, 0, BUF_SIZE);
        // printf("Connected to server\n");
        if (activity < 0)
        {
            printf("\nSelect error \n");
            return -1;
        }
        if (FD_ISSET(sock, &read_fds))
        {
            memset(buffer, 0, BUF_SIZE);
            printf("Write: ");
            fgets(buffer, BUF_SIZE, stdin);
            // Send input to server socket
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, BUF_SIZE);
            // Receive data from server socket
            valread = read(sock, buffer, BUF_SIZE);
            if (valread == 0)
            {
                // Server disconnected
                printf("Server disconnected\n");
                break;
            }
            // Output received data to stdout
            printf("Answer: %s\n", buffer);
        }
    }

    // Close socket
    close(sock);

    return 0;
}
int server(int argc, char *argv[])
{
    int max_clients = 1;
    int client_sockets[max_clients];
    fd_set read_fds;
    int max_fd;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUF_SIZE] = {0};

    if (argc != 3)
    {
        printf("\nUsage: %s -s PORT\n", argv[0]);
        return -1;
    }

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("\nSocket creation error \n");
        return -1;
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | 15, &opt, sizeof(opt)))
    {
        printf("\nSetsockopt error \n");
        return -1;
    }

    memset(&address, '0', sizeof(address));

    // Set socket address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[2]));

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("\nBind failed \n");
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        printf("\nListen error \n");
        return -1;
    }

    // Initialize client sockets array
    for (int i = 0; i < max_clients; i++)
    {
        client_sockets[i] = 0;
    }

    // Add server socket to read_fds set
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    max_fd = server_fd;
    int flagClientServer = 0;
    while (1)
    {
        // Wait for activity on any of the sockets
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0)
        {
            printf("\nSelect error \n");
            return -1;
        }

        // Check for activity on server socket
        if (FD_ISSET(server_fd, &read_fds))
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                printf("\nAccept error \n");
                return -1;
            }

            // Add new client socket to client_sockets array
            for (int i = 0; i < max_clients; i++)
            {
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    break;
                }
            }

            // Add new client socket to read_fds set
            FD_SET(new_socket, &read_fds);

            // Update max_fd if necessary
            if (new_socket > max_fd)
            {
                max_fd = new_socket;
            }

            printf("New client connected\n");
        }

        // Check for activity on client sockets
        for (int i = 0; i < max_clients; i++)
        {
            int client_socket = client_sockets[i];

            if (FD_ISSET(client_socket, &read_fds))
            {
                // Receive data from client socket
                if (flagClientServer == 0)
                    send(client_socket, "start", strlen("start"), 0);
                flagClientServer = 1;
                int valread = read(client_socket, buffer, BUF_SIZE);
                // printf("%s\n",buffer);
                if (valread == 0)
                {
                    // Client disconnected
                    printf("Client disconnected\n");

                    // Remove client socket from client_sockets array
                    client_sockets[i] = 0;

                    // Remove client socket from read_fds set
                    FD_CLR(client_socket, &read_fds);

                    // Close client socket
                    close(client_socket);
                }
                else
                {
                    // Output received data to stdout
                    printf("Answer: %s\n", buffer);
                    // Read input from stdin
                    memset(buffer, 0, BUF_SIZE);
                    printf("Write: ");
                    fgets(buffer, BUF_SIZE, stdin);
                    // Send input to client socket
                    send(client_socket, buffer, strlen(buffer), 0);
                    memset(buffer, 0, BUF_SIZE);
                }
            }
        }
    }

    // Close server socket
    close(server_fd);

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
        printf("Please enter an input in the right format.");
    }
    return 0;
}
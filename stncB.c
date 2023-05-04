#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <ctype.h>
#include <sys/un.h>

#define BUF_SIZE 8192
#define SOCKET_PATH "/tmp/my_socket.sock"
#define SERVER_SOCKET_PATH "/tmp/uds_dgram_server"
#define CLIENT_SOCKET_PATH "/tmp/uds_dgram_client"

int ipv4_tcp_client(int argc, char *argv[]);
int ipv4_tcp_server(int argc, char *argv[]);
int ipv4_udp_client(int argc, char *argv[]);
int ipv4_udp_server(int argc, char *argv[]);
int ipv6_tcp_client(int argc, char *argv[]);
int ipv6_tcp_server(int argc, char *argv[]);
int ipv6_udp_client(int argc, char *argv[]);
int ipv6_udp_server(int argc, char *argv[]);
int uds_stream_client(int argc, char *argv[]);
int uds_stream_server(int argc, char *argv[]);
int uds_dgram_client(int argc, char *argv[]);
int uds_dgram_server(int argc, char *argv[]);

int ipv4_tcp_client(int argc, char *argv[])
{

    int sock = 0, valread = -1;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE] = {0};
    fd_set read_fds;

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
    // int activity = select(sock + 1, &read_fds, NULL, NULL, NULL);
    while (1)
    {
        if (flagClient == 0)
        {
            read(sock, buffer, BUF_SIZE);
        }
        flagClient = 1;
        memset(buffer, 0, BUF_SIZE);
        // printf("Connected to server\n");
        //  if (activity < 0) {
        //      printf("\nSelect error \n");
        //      return -1;
        //  }
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
int ipv4_tcp_server(int argc, char *argv[])
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
int ipv4_udp_client(int argc, char *argv[])
{
    int sock = 0, valread = -1;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE] = {0};
    socklen_t addr_len = sizeof(serv_addr);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
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

    printf("Connected to server\n");

    while (1)
    {
        memset(buffer, 0, BUF_SIZE);
        printf("Write: ");
        fgets(buffer, BUF_SIZE, stdin);
        // Send input to server socket
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, addr_len);

        memset(buffer, 0, BUF_SIZE);
        // Receive data from server socket
        valread = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *)&serv_addr, &addr_len);
        if (valread == 0)
        {
            // Server disconnected
            printf("Server disconnected\n");
            break;
        }
        // Output received data to stdout
        printf("Answer: %s\n", buffer);
    }

    // Close socket
    close(sock);

    return 0;
}
int ipv4_udp_server(int argc, char *argv[])
{
    int server_fd;
    struct sockaddr_in address, cli_addr;
    socklen_t addr_len = sizeof(cli_addr);
    char buffer[BUF_SIZE] = {0};

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        printf("\nSocket creation error \n");
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

    while (1)
    {
        memset(buffer, 0, BUF_SIZE);
        // Receive data from client socket
        int valread = recvfrom(server_fd, buffer, BUF_SIZE, 0, (struct sockaddr *)&cli_addr, &addr_len);
        if (valread == 0)
        {
            // Client disconnected
            printf("Client disconnected\n");
            break;
        }
        // Output received data to stdout
        printf("Received: %s\n", buffer);
        memset(buffer, 0, BUF_SIZE);
        printf("Write: ");
        fgets(buffer, BUF_SIZE, stdin);
        sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, addr_len);
    }

    close(server_fd);
    return 0;
}
int ipv6_tcp_client(int argc, char *argv[])
{

    int sock = 0, valread = -1;
    struct sockaddr_in6 server_addr;
    char buffer[BUF_SIZE] = {0};
    fd_set read_fds;

    // Create socket
    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&server_addr, '0', sizeof(server_addr));

    // Set socket address
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(atoi(argv[3]));

    // Convert IPv4 and store in sin_addr
    if (inet_pton(AF_INET6, argv[2], &server_addr.sin6_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server socket
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
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
int ipv6_tcp_server(int argc, char *argv[])
{
    int max_clients = 1;
    int client_sockets[max_clients];
    fd_set read_fds;
    int max_fd;
    int server_fd, new_socket;
    struct sockaddr_in6 address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUF_SIZE] = {0};

    // Create server socket
    if ((server_fd = socket(AF_INET6, SOCK_STREAM, 0)) == 0)
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
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(atoi(argv[2]));

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("\nBind failed \n");
        return -1;
    }
    // printf("listening\n");
    //  Listen for incoming connections
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
            // printf("done listening\n");

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
int ipv6_udp_client(int argc, char *argv[])
{
    int sock = 0, valread = -1;
    struct sockaddr_in6 serv_addr;
    char buffer[BUF_SIZE] = {0};
    socklen_t addr_len = sizeof(serv_addr);

    // Create socket
    if ((sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    // Set socket address
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(atoi(argv[3]));

    // Convert IPv4 and store in sin_addr
    if (inet_pton(AF_INET6, argv[2], &serv_addr.sin6_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    printf("Connected to server\n");

    while (1)
    {
        memset(buffer, 0, BUF_SIZE);
        printf("Write: ");
        fgets(buffer, BUF_SIZE, stdin);
        // Send input to server socket
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, addr_len);

        memset(buffer, 0, BUF_SIZE);
        // Receive data from server socket
        valread = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr *)&serv_addr, &addr_len);
        if (valread == 0)
        {
            // Server disconnected
            printf("Server disconnected\n");
            break;
        }
        // Output received data to stdout
        printf("Answer: %s\n", buffer);
    }

    // Close socket
    close(sock);

    return 0;
}
int ipv6_udp_server(int argc, char *argv[])
{
    int server_fd;
    struct sockaddr_in6 address, cli_addr;
    socklen_t addr_len = sizeof(cli_addr);
    char buffer[BUF_SIZE] = {0};

    // Create server socket
    if ((server_fd = socket(AF_INET6, SOCK_DGRAM, 0)) == 0)
    {
        printf("\nSocket creation error \n");
        return -1;
    }

    memset(&address, '0', sizeof(address));

    // Set socket address
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(atoi(argv[2]));

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("\nBind failed \n");
        return -1;
    }

    while (1)
    {
        memset(buffer, 0, BUF_SIZE);
        // Receive data from client socket
        int valread = recvfrom(server_fd, buffer, BUF_SIZE, 0, (struct sockaddr *)&cli_addr, &addr_len);
        if (valread == 0)
        {
            // Client disconnected
            printf("Client disconnected\n");
            break;
        }
        // Output received data to stdout
        printf("Received: %s\n", buffer);
        memset(buffer, 0, BUF_SIZE);
        printf("Write: ");
        fgets(buffer, BUF_SIZE, stdin);
        sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, addr_len);
    }

    close(server_fd);
    return 0;
}
int uds_stream_client(int argc, char *argv[])
{
    int sock, valread;
    struct sockaddr_un server_address;
    char buffer[BUF_SIZE];

    // Create socket
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("Failed to create client socket\n");
        return -1;
    }
    memset(&server_address, 0, sizeof(struct sockaddr_un));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, SOCKET_PATH, sizeof(server_address.sun_path) - 1);
    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Failed to connect to server\n");
        return -1;
    }

    printf("Connected to server\n");
    while (1)
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

    // Close socket
    close(sock);

    return 0;
}
int uds_stream_server(int argc, char *argv[])
{
    int server_fd, client_fd;
    struct sockaddr_un address;
    char buffer[BUF_SIZE];

    // Create server socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("Failed to create server socket\n");
        return -1;
    }
    remove(SOCKET_PATH);
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Failed to bind server socket to address\n");
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1)
    {
        printf("Failed to listen for incoming connections\n");
        return -1;
    }

    printf("Server is listening for incoming connections...\n");

    // Accept incoming connections
    if ((client_fd = accept(server_fd, NULL, NULL)) == -1)
    {
        printf("Failed to accept incoming connection\n");
        return -1;
    }

    printf("Client connected to server\n");

    while (1)
    {
        memset(buffer, 0, BUF_SIZE);

        // Receive data from client socket
        int valread = recv(client_fd, buffer, BUF_SIZE, 0);

        if (valread == 0)
        {
            // Client disconnected
            printf("Client disconnected\n");
            break;
        }

        // Output received data to stdout
        printf("Received: %s\n", buffer);

        memset(buffer, 0, BUF_SIZE);
        printf("Write: ");
        fgets(buffer, BUF_SIZE, stdin);

        // Send input to client socket
        send(client_fd, buffer, strlen(buffer), 0);
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    return 0;
}
int uds_dgram_client(int argc, char *argv[])
{
    int send_sock, recv_sock;
    struct sockaddr_un server_address, client_address;
    char send_buffer[BUF_SIZE], recv_buffer[BUF_SIZE];

    // Create sending socket
    if ((send_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        printf("Failed to create sending socket\n");
        return -1;
    }
    memset(&server_address, 0, sizeof(struct sockaddr_un));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, SERVER_SOCKET_PATH, sizeof(server_address.sun_path) - 1);

    // Create receiving socket
    if ((recv_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        printf("Failed to create receiving socket\n");
        return -1;
    }
    memset(&client_address, 0, sizeof(struct sockaddr_un));
    client_address.sun_family = AF_UNIX;
    strncpy(client_address.sun_path, CLIENT_SOCKET_PATH, sizeof(client_address.sun_path) - 1);
    remove(client_address.sun_path);
    if (bind(recv_sock, (struct sockaddr *)&client_address, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Failed to bind receiving socket to address\n");
        return -1;
    }

    printf("Client started\n");
    while (1)
    {
        memset(send_buffer, 0, BUF_SIZE);
        printf("Write: ");
        fgets(send_buffer, BUF_SIZE, stdin);

        // Send input to server socket
        sendto(send_sock, send_buffer, strlen(send_buffer), 0, (struct sockaddr *)&server_address, sizeof(struct sockaddr_un));

        memset(recv_buffer, 0, BUF_SIZE);
        socklen_t len = sizeof(struct sockaddr_un);

        // Receive data from server socket
        int valread = recvfrom(recv_sock, recv_buffer, BUF_SIZE, 0, (struct sockaddr *)&server_address, &len);
        if (valread == -1)
        {
            printf("Failed to receive message\n");
            continue;
        }

        // Output received data to stdout
        printf("Answer: %s\n", recv_buffer);
    }

    // Close sockets
    close(send_sock);
    close(recv_sock);
    unlink(CLIENT_SOCKET_PATH);

    return 0;
}
int uds_dgram_server(int argc, char *argv[])
{
    int server_fd, client_fd, len;
    struct sockaddr_un server_addr, client_addr;
    char buffer[BUF_SIZE];

    // Create server socket
    if ((server_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        printf("Failed to create server socket\n");
        return -1;
    }

    remove(SERVER_SOCKET_PATH);
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SERVER_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Bind server socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Failed to bind server socket to address\n");
        return -1;
    }

    printf("Server is waiting for incoming messages...\n");

    while (1)
    {
        memset(buffer, 0, BUF_SIZE);

        // Receive message from client socket
        len = sizeof(struct sockaddr_un);
        if (recvfrom(server_fd, buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, (socklen_t *restrict)&len) == -1)
        {
            printf("Failed to receive message from client\n");
            return -1;
        }

        // Output received message to stdout
        printf("Received message: %s\n", buffer);

        // Send response to client socket
        if ((client_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
        {
            printf("Failed to create client socket\n");
            return -1;
        }
        memset(&client_addr, 0, sizeof(struct sockaddr_un));
        client_addr.sun_family = AF_UNIX;
        strncpy(client_addr.sun_path, CLIENT_SOCKET_PATH, sizeof(client_addr.sun_path) - 1);
        memset(buffer, 0, BUF_SIZE);
        printf("Write: ");
        fgets(buffer, BUF_SIZE, stdin);
        if (sendto(client_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_un)) == -1)
        {
            printf("Failed to send message to client\n");
            return -1;
        }

        close(client_fd);
    }

    close(server_fd);
    unlink(SERVER_SOCKET_PATH);

    return 0;
}
int main(int argc, char *argv[])
{

    if (argc < 4 || argc > 7)
    {
        puts("Incorrect number of arguments\n");
        printf("Server Usage: ...\n");
        printf("Client Usage: ...\n");
        return -1;
    }

    if (!strcmp(argv[1], "-c"))
    {
        if (!strcmp(argv[4], "-p"))
        {
            if (!strcmp(argv[5], "ipv4"))
            {
                if (!strcmp(argv[6], "tcp"))
                {
                    ipv4_tcp_client(argc, argv);
                }
                else if (!strcmp(argv[6], "udp"))
                {
                    ipv4_udp_client(argc, argv);
                }
            }
            else if (!strcmp(argv[5], "ipv6"))
            {
                if (!strcmp(argv[6], "tcp"))
                {
                    ipv6_tcp_client(argc, argv);
                }
                else if (!strcmp(argv[6], "udp"))
                {
                    ipv6_udp_client(argc, argv);
                }
            }
            else if (!strcmp(argv[5], "uds"))
            {
                if (!strcmp(argv[6], "stream"))
                {
                    uds_stream_client(argc, argv);
                }
                else if (!strcmp(argv[6], "dgram"))
                {
                    uds_dgram_client(argc, argv);
                }
            }
            else if (!strcmp(argv[5], "mmap"))
            {
            }
            else if (!strcmp(argv[5], "pipe"))
            {
            }
        }
    }
    else if (!strcmp(argv[1], "-s"))
    {
        ipv4_tcp_server(argc, argv);
    }
    else
    {
        puts("Incorrect arguments\n");
        printf("Server Usage: ...\n");
        printf("Client Usage: ...\n");
    }
    return 0;
}

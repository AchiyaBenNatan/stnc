#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SHM_NAME "/mySharedMemory"
#define BUF_SIZE 64000
#define DATA_SIZE 1048575
#define SOCKET_PATH "/tmp/my_socket.sock"
#define SERVER_SOCKET_PATH "/tmp/uds_dgram_server"
#define CLIENT_SOCKET_PATH "/tmp/uds_dgram_client"
#define FILENAME "file2.txt"
enum addr
{
    IPV4,
    IPV6
};

char *generate_rand_str(int length);
unsigned short calculate_checksum(unsigned short *paddress, int len);
int tcp_client(int argc, char *argv[], enum addr type);
int tcp_server(int argc, char *argv[], enum addr type);
int udp_client(int argc, char *argv[], enum addr type);
int udp_server(int argc, char *argv[], enum addr type);
int uds_stream_client(int argc, char *argv[]);
int uds_stream_server(int argc, char *argv[]);
int uds_dgram_client(int argc, char *argv[]);
int uds_dgram_server(int argc, char *argv[]);

int tcp_client(int argc, char *argv[], enum addr type)
{
    int sock = 0;
    int sendStream = 0, totalSent = 0;
    char buffer[BUF_SIZE] = {0};
    struct sockaddr_in serv_addr;
    struct sockaddr_in6 serv_addr6;
    struct timeval start, end;
    enum addr addrType = (type == 0) ? IPV4 : IPV6;

    if (addrType == IPV4)
    {
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
    }
    else if (addrType == IPV6)
    {
        // Create socket
        if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return -1;
        }

        memset(&serv_addr, '0', sizeof(serv_addr6));

        // Set socket address
        serv_addr6.sin6_family = AF_INET6;
        serv_addr6.sin6_port = htons(atoi(argv[3]));

        // Convert IPv4 and store in sin_addr
        if (inet_pton(AF_INET6, argv[2], &serv_addr6.sin6_addr) <= 0)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }

        // Connect to server socket
        if (connect(sock, (struct sockaddr *)&serv_addr6, sizeof(serv_addr6)) < 0)
        {
            printf("\nConnection Failed \n");
            return -1;
        }
    }
    else
    {
        printf("Invalid address type\n");
        return -1;
    }

    printf("Connected to server\n");

    // Generate data
    char * data = generate_rand_str(DATA_SIZE);

    // Calculate and send checksum
    unsigned short checksum = calculate_checksum((unsigned short *)data, strlen(data));
    unsigned short checksum_net = htons(checksum);
    memcpy(buffer, &checksum_net, sizeof(checksum_net));
    sendStream = send(sock, buffer, sizeof(checksum_net), 0);
    if (-1 == sendStream)
    {
        printf("send() failed");
        exit(1);
    }

    // Send data
    gettimeofday(&start, 0);
    while (totalSent < strlen(data))
    {
        int bytes_to_read = (BUF_SIZE < strlen(data) - totalSent) ? BUF_SIZE: strlen(data) - totalSent;
        memcpy(buffer, data + totalSent, bytes_to_read);
        sendStream = send(sock, buffer, bytes_to_read, 0);
        if (-1 == sendStream)
        {
            printf("send() failed");
            exit(1);
        }

        totalSent += sendStream;
        //printf("Bytes sent: %d\n", totalSent);
        //printf ("bytes to read: %d\n", bytes_to_read);
        sendStream = 0;
        bzero(buffer, sizeof(buffer));
    }
    gettimeofday(&end, 0);
    unsigned long miliseconds = (end.tv_sec - start.tv_sec) * 1000 + end.tv_usec - start.tv_usec / 1000;
    printf("Total bytes sent: %d\nTime elapsed: %lu miliseconds\n", totalSent, miliseconds);

    // Close socket
    close(sock);
    free(data);
    return 0;
}

int tcp_server(int argc, char *argv[], enum addr type)
{
    int ServerSocket, ClientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    struct sockaddr_in6 serverAddr6, clientAddr6;
    socklen_t clientAddressLen;
    int opt = 1, bytes = 0, countbytes = 0;
    char buffer[BUF_SIZE] = {0}, totalData[DATA_SIZE] = {0};
    enum addr addrType = (type == 0) ? IPV4 : IPV6;

    if (addrType == IPV4)
    {
        // Create server socket
        if ((ServerSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            printf("\nSocket creation error \n");
            return -1;
        }

        memset(&serverAddr, '0', sizeof(serverAddr));
        memset(&clientAddr, '0', sizeof(clientAddr));
        clientAddressLen = sizeof(clientAddr);

        // Set socket address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(atoi(argv[2]));

        // Bind socket to address
        if (bind(ServerSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            printf("\nBind failed \n");
            return -1;
        }
    }
    else if (addrType == IPV6)
    {
        // Create server socket
        if ((ServerSocket = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
        {
            printf("\nSocket creation error \n");
            return -1;
        }

        memset(&serverAddr6, '0', sizeof(serverAddr6));
        memset(&clientAddr6, '0', sizeof(clientAddr6));
        clientAddressLen = sizeof(clientAddr6);

        // Set socket address
        serverAddr6.sin6_family = AF_INET6;
        serverAddr6.sin6_addr = in6addr_any;
        serverAddr6.sin6_port = htons(atoi(argv[2]));

        // Bind socket to address
        if (bind(ServerSocket, (struct sockaddr *)&serverAddr6, sizeof(serverAddr6)) < 0)
        {
            printf("\nBind failed \n");
            return -1;
        }
    }
    else
    {
        printf("Invalid address type\n");
        return -1;
    }

    // Listen for incoming connections
    if (listen(ServerSocket, 3) < 0)
    {
        printf("\nListen error \n");
        return -1;
    }
    printf("Server is listening for incoming connections...\n");

    // Set socket options
    if (setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR | 15, &opt, sizeof(opt)))
    {
        printf("\nSetsockopt error \n");
        return -1;
    }

    if (addrType == IPV4)
    {
        if ((ClientSocket = accept(ServerSocket, (struct sockaddr *)&clientAddr, &clientAddressLen)) < 0)
        {
            printf("\nAccept error \n");
            return -1;
        }
    }
    else if (addrType == IPV6)
    {
        if ((ClientSocket = accept(ServerSocket, (struct sockaddr *)&clientAddr6, &clientAddressLen)) < 0)
        {
            printf("\nAccept error \n");
            return -1;
        }
    }

    printf("Client connected\n");

    // Receive checksum
    unsigned short received_checksum;
    if ((bytes = recv(ClientSocket, &received_checksum, sizeof(received_checksum), 0)) < 0)
    {
        printf("recv failed. Sender inactive.\n");
        close(ServerSocket);
        close(ClientSocket);
        return -1;
    }
    received_checksum = ntohs(received_checksum);

    while (1)
    {
        if ((bytes = recv(ClientSocket, buffer, sizeof(buffer), 0)) < 0)
        {
            printf("recv failed. Sender inactive.\n");
            close(ServerSocket);
            close(ClientSocket);
            return -1;
        }
        else if (countbytes && bytes == 0)
        {
            printf("Total bytes received: %d\n", countbytes);
            break;
        }

        memcpy(totalData + countbytes, buffer, bytes);
        //printf("bytes: %d\n", bytes);
        //printf("totaldata: %ld\n", strlen(totalData));
        countbytes += bytes;
    }

    // Calculate checksum
    unsigned short calculated_checksum = calculate_checksum((unsigned short *)totalData, strlen(totalData));
    if (calculated_checksum == received_checksum)
    {
        printf("Checksums match\n");
    }
    else
    {
        printf("Checksums don't match\n");
    }

    // Close server socket
    close(ServerSocket);
    close(ClientSocket);
    return 0;
}

int udp_client(int argc, char *argv[], enum addr type)
{
    FILE *fp;
    int sock = 0;
    int dataGram = -1, sendStream = 0, totalSent = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in6 serv_addr6;
    char buffer[BUF_SIZE] = {0};
    struct timeval start, end;
    const char *endMsg = "FILEEND";
    enum addr addrType = (type == 0) ? IPV4 : IPV6;

    if (addrType == IPV4)
    {
        // Create socket
        if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
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
    }
    else if (addrType == IPV6)
    {
        // Create socket
        if ((sock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return -1;
        }

        memset(&serv_addr6, '0', sizeof(serv_addr6));

        // Set socket address
        serv_addr6.sin6_family = AF_INET6;
        serv_addr6.sin6_port = htons(atoi(argv[3]));

        // Convert IPv4 and store in sin_addr
        if (inet_pton(AF_INET6, argv[2], &serv_addr6.sin6_addr) <= 0)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }
    }
    else
    {
        printf("Invalid address type\n");
        return -1;
    }

    fp = fopen(FILENAME, "rb");
    if (fp == NULL)
    {
        printf("fopen() failed\n");
        exit(1);
    }

    printf("Connected to server\n");

    gettimeofday(&start, 0);
    while ((dataGram = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0)
    {
        if (addrType == IPV4)
        {
            sendStream = sendto(sock, buffer, dataGram, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        }
        else if (addrType == IPV6)
        {
            sendStream = sendto(sock, buffer, dataGram, 0, (struct sockaddr *)&serv_addr6, sizeof(serv_addr6));
        }

        if (-1 == sendStream)
        {
            printf("send() failed");
            exit(1);
        }

        totalSent += sendStream;
        // printf("Bytes sent: %f\n", totalSent);
        // printf("location in file %ld\n", ftell(fp));
        sendStream = 0;
        bzero(buffer, sizeof(buffer));
    }

    gettimeofday(&end, 0);
    strcpy(buffer, endMsg);
    if (addrType == IPV4)
    {
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    }
    else if (addrType == IPV6)
    {
        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr6, sizeof(serv_addr6));
    }

    unsigned long miliseconds = (end.tv_sec - start.tv_sec) * 1000 + end.tv_usec - start.tv_usec / 1000;
    printf("Total bytes sent: %d\nTime elapsed: %lu miliseconds\n", totalSent, miliseconds);

    // Close socket
    close(sock);

    return 0;
}

int udp_server(int argc, char *argv[], enum addr type)
{
    int ServerSocket;
    struct sockaddr_in serverAddr, clientAddr;
    struct sockaddr_in6 serverAddr6, clientAddr6;
    socklen_t clientAddressLen;
    int bytes = 0, countbytes = 0;
    char buffer[BUF_SIZE] = {0}, decoded[BUF_SIZE];
    enum addr addrType = (type == 0) ? IPV4 : IPV6;

    if (addrType == IPV4)
    {
        // Create server socket
        if ((ServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == 0)
        {
            printf("\nSocket creation error \n");
            return -1;
        }

        memset(&serverAddr, '0', sizeof(serverAddr));
        memset(&clientAddr, '0', sizeof(clientAddr));

        // Set socket address
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(atoi(argv[2]));

        // Bind socket to address
        if (bind(ServerSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            printf("\nBind failed \n");
            return -1;
        }
    }
    else if (addrType == IPV6)
    {
        // Create server socket
        if ((ServerSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == 0)
        {
            printf("\nSocket creation error \n");
            return -1;
        }

        memset(&serverAddr6, '0', sizeof(serverAddr6));
        memset(&clientAddr6, '0', sizeof(clientAddr6));

        // Set socket address
        serverAddr6.sin6_family = AF_INET6;
        serverAddr6.sin6_addr = in6addr_any;
        serverAddr6.sin6_port = htons(atoi(argv[2]));

        // Bind socket to address
        if (bind(ServerSocket, (struct sockaddr *)&serverAddr6, sizeof(serverAddr6)) < 0)
        {
            printf("\nBind failed \n");
            return -1;
        }
    }
    else
    {
        printf("Invalid address type\n");
        return -1;
    }

    printf("Server is listening for incoming messages...\n");

    while (1)
    {
        if (addrType == IPV4)
        {
            if ((bytes = recvfrom(ServerSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clientAddressLen)) < 0)
            {
                printf("recv failed. Sender inactive.\n");
                close(ServerSocket);
                return -1;
            }
        }
        else if (addrType == IPV6)
        {
            if ((bytes = recvfrom(ServerSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr6, &clientAddressLen)) < 0)
            {
                printf("recv failed. Sender inactive.\n");
                close(ServerSocket);
                return -1;
            }
        }

        if (bytes < 10)
        {
            buffer[bytes] = '\0';
            strncpy(decoded, buffer, sizeof(decoded));
            if (strcmp(decoded, "FILEEND") == 0)
            {
                printf("Total bytes received: %d\n", countbytes);
                break;
            }
        }

        countbytes += bytes;
    }

    close(ServerSocket);
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
int mmap_client(int argc, char *argv[])
{
    int fd = open(argv[6], O_RDONLY);
    if (fd == -1)
    {
        printf("open() failed\n");
        return -1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        printf("fstat() failed\n");
        return -1;
    }
    off_t len = sb.st_size;
    void *addr = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
    {
        printf("mmap() failed\n");
        return -1;
    }
    close(fd);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        printf("shm_open() failed\n");
        return -1;
    }
    if (ftruncate(shm_fd, len) == -1)
    {
        printf("ftruncate() failed\n");
        return -1;
    }
    void *shm_addr = mmap(NULL, len, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_addr == MAP_FAILED)
    {
        printf("mmap() failed\n");
        return -1;
    }
    memcpy(shm_addr, addr, len);
    munmap(addr, len);
    munmap(shm_addr, len);
    close(shm_fd);
    return 0;
}

int mmap_server(int argc, char *argv[])
{

    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1)
    {
        printf("shm_open() failed\n");
        return -1;
    }
    struct stat sb;
    if (fstat(shm_fd, &sb) == -1)
    {
        printf("fstat() failed\n");
        return -1;
    }
    off_t len = sb.st_size;

    void *addr = mmap(NULL, len, PROT_READ, MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    if (addr == MAP_FAILED)
    {
        printf("mmap() failed\n");
        return -1;
    }
    // fwrite(addr, len, 1, stdout);
    printf("Shared memory size: %ld\n", len);
    munmap(addr, len);

    shm_unlink(SHM_NAME);
    return 0;
}

int main(int argc, char *argv[])
{

    if (argc < 3 || argc > 7)
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
                    tcp_client(argc, argv, IPV4);
                }
                else if (!strcmp(argv[6], "udp"))
                {
                    udp_client(argc, argv, IPV4);
                }
            }
            else if (!strcmp(argv[5], "ipv6"))
            {
                if (!strcmp(argv[6], "tcp"))
                {
                    tcp_client(argc, argv, IPV6);
                }
                else if (!strcmp(argv[6], "udp"))
                {
                    udp_client(argc, argv, IPV6);
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
                mmap_client(argc, argv);
            }
            else if (!strcmp(argv[5], "pipe"))
            {
            }
        }
    }
    else if (!strcmp(argv[1], "-s"))
    {
        tcp_server(argc, argv, IPV4);
    }
    else
    {
        puts("Incorrect arguments\n");
        printf("Server Usage: ...\n");
        printf("Client Usage: ...\n");
    }
    return 0;
}

unsigned short calculate_checksum(unsigned short *paddress, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = paddress;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *((unsigned char *)&answer) = *((unsigned char *)w);
        sum += answer;
    }

    // add back carry outs from top 16 bits to low 16 bits
    sum = (sum >> 16) + (sum & 0xffff); // add hi 16 to low 16
    sum += (sum >> 16);                 // add carry
    answer = ~sum;                      // truncate to 16 bits

    return answer;
}

char *generate_rand_str(int length)
{
    char *string = malloc(length + 1);
    if (!string)
    {
        return NULL;
    }

    for (int i = 0; i < length; i++)
    {
        int num = rand() % 26;
        string[i] = 'a' + num;
    }
    string[length] = '\0';
    return string;
}
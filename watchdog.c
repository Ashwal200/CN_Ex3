#include <stdio.h>


// Linux and other UNIXes
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#define SERVER_PORT 3000

int gettimeofday();

int main(int argc, char *argv[])
{   
      // Create a TCP Connection between the sender and receiver. As known, it doesn’t create a
    // whole new socket with TCP connection. Its just should establish connection with the socket
    // that the sender created.
    // signal(SIGPIPE, SIG_IGN);
    // on linux to prevent crash on closing socket
    // Open the listening (server) socket
    int listening_socket = -1;
    // 0 means default protocol for stream sockets (Equivalently, IPPROTO_TCP)
    listening_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
    if (listening_socket == -1)
    {
        printf("Could not create listening socket : %d", errno);
        return 1;
    }

     // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    //
    int enable_reuse = 1;
    int ret = setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(int));
    if (ret < 0) {
        printf("setsockopt() failed with error code : %d", errno);
        return 1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication.
    struct sockaddr_in ping_address;
    memset(&ping_address, 0, sizeof(ping_address));
    // This field contains the address family, which is always AF_INET when we use TCP.
    ping_address.sin_family = AF_INET;
    // Any IP at this port (Address to accept any incoming messages).
    ping_address.sin_addr.s_addr = INADDR_ANY;
    // It refers to a 16-bit port number on which the Receiver will listen to the connection requests by the clients.
    // Function htons convert (5001 = 0x89 0x13) little endian => (0x13 0x89) network endian (big endian).
    ping_address.sin_port = htons(SERVER_PORT);

    // Bind the socket to the port with any IP at this port.
    int bind_result = bind(listening_socket, (struct sockaddr *)&ping_address, sizeof(ping_address));
   // If could not bind the socket to the port.
    if (bind_result == -1)
    {
        printf("Bind failed with error code : %d\n", errno);
        // close the socket
        close(listening_socket);
        return -1;
    }

    printf("Bind() success\n");

    // Make the socket listening; actually mother of all client sockets.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections
    int listen_result = listen(listening_socket, 3);
    if (listen_result == -1)
    {
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(listening_socket);
        return -1;
    }
    // Get a connection from the sender.
    // Accept and incoming connection.
    printf("Waiting for incoming TCP-connections...\n");
    
    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication.
    // This structure allows you to bind a socket with the desired.
    struct sockaddr_in sender_address;
   
    // Unsigned opaque integral type of length of at least 32 bits. 
    socklen_t sender_address_len = sizeof(sender_address);
   
    // Copies the character 0 (an unsigned char) to the string pointed by the argument sender_address.
    // By the length of sizeof(sender_address) characters.
    memset(&sender_address, 0, sizeof(sender_address));
    sender_address_len = sizeof(sender_address);
 
   
// Accept the connection from the server.
    int socket_ping = accept(listening_socket, (struct sockaddr *)&sender_address, &sender_address_len);
    if (socket_ping == -1)
    {
        printf("listen failed with error code : %d\n", errno);
        // close the sockets
        close(listening_socket);
        return -1;
    }
    printf("A new client connection accepted\n");
    
    fcntl(listening_socket, F_SETFL, O_NONBLOCK);
    
    fcntl(socket_ping, F_SETFL, O_NONBLOCK);

//for calculate the time
    struct timeval start, end;
    float timer = 0;
   
    sleep(1);
    //checking if watchdog receive something from ping if yes then update the start time if no then the time will stop updating and then second>10 -> shut down
    while (timer <= 10){
        int ping_start = 0;
        int receive= recv(socket_ping, &ping_start, sizeof(int), 0);
        if(receive>0){
            gettimeofday(&start, 0);
        }
        gettimeofday(&end, 0);
        timer = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0f;
    }

    printf("server <%s> cannot be reached.\n", argv[1]);

    kill(getppid() , SIGKILL);
    return 0;
    



}
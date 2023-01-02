#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

// ICMP header len for echo req
#define ICMP_HDRLEN 8 
// watchdog port by request
#define WATCHDOG_PORT 3000
#define SERVER_IP_ADDRESS "127.0.0.1"
// 1. Change SOURCE_IP and DESTINATION_IP to the relevant
//     for your computer
// 2. Compile it using MSVC compiler or g++
// 3. Run it from the account with administrative permissions,
//    since opening of a raw-socket requires elevated preveledges.
//
//    On Windows, right click the exe and select "Run as administrator"
//    On Linux, run it as a root or with sudo.
//
// 4. For debugging and development, run MS Visual Studio (MSVS) as admin by
//    right-clicking at the icon of MSVS and selecting from the right-click
//    menu "Run as administrator"
//
//  Note. You can place another IP-source address that does not belong to your
//  computer (IP-spoofing), i.e. just another IP from your subnet, and the ICMP
//  still be sent, but do not expect to see ICMP_ECHO_REPLY in most such cases
//  since anti-spoofing is wide-spread.



// Compute checksum (RFC 1071).
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

    // add back carry-outs from top 16 bits to low 16 bits
    sum = (sum >> 16) + (sum & 0xffff); 
    // add hi 16 to low 16
    sum += (sum >> 16);                 
    // add carry
    answer = ~sum;                      
    // truncate to 16 bits

    return answer;
}

int validate_number(char *str) {
    while (*str) {
        //if the character is not a number, return
        if(!isdigit(*str)){ 
            false;
            return 0;
        }
        //point to next character
        str++; 
    }
    return 1;
}
int validate_Ip(char *ip) { 
    //check whether the IP is valid or not
    int i, num, dots = 0;
    char *ptr;
    char *temp = ip;
    if (temp == NULL)
        return 0;
    ptr = strtok(temp, "."); 
    //cut the string using dor delimiter
    if (ptr == NULL)
        return 0;
    while (ptr) {
        if (!validate_number(ptr)) 
        //check whether the sub string is holding only number or not
            return 0;
        num = atoi(ptr); 
        //convert substring to number
        if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, "."); 
            //cut the next part of the string
            if (ptr != NULL)
                dots++; 
                //increase the dot count
        } else
            return 0;
    }
    if (dots != 3) 
    //if the number of dots are not 3, return false
        return 0;
    return 1;
}


int main(int argc, char *argv[]){
    //check the IP
    if (argc != 2) {
        perror("you need to put IP!");
        exit(-1);
    }
    char ptr_IP[15];
    strcpy(ptr_IP , argv[1]);
    int flage = validate_Ip(ptr_IP);
    if (flage == 0)
    {
        printf("Ip isn't valid!\n");
        exit(-1);
    }

    // run 2 programs using fork + exec
    // command: make clean && make all && ./partb
    char *args[2];
    // compiled watchdog.c by makefile
    args[0] = "./watchdog";
    args[1] = argv[1];
    int status;
    int pid = fork();
    if (pid == 0)
    {
        printf("in child \n");
        execvp(args[0], args);
        // alarm(10);
    }

    //open a socket for watchdog
    int watchdog_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (watchdog_socket == -1) {
        printf("Could not create socket : %d\n", errno);
        return -1;
    } 
    else 
        printf("New socket opened\n");
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(WATCHDOG_PORT);
    int rval = inet_pton(AF_INET, (const char *) SERVER_IP_ADDRESS, &server_address.sin_addr);
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    }

    sleep(5);

    // Make a connection with watchdog
    int connect_result = connect(watchdog_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    if (connect_result == -1) {
        printf("connect() failed with error code : %d\n", errno);
        close(watchdog_socket);
        return -1;
    } else printf("connected to watchdog\n");

    //open socket for ping
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
        fprintf(stderr, "socket() failed with error: %d", errno);
        fprintf(stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
        return -1;
    }
    struct sockaddr_in dest_in;
    memset(&dest_in, 0, sizeof(struct sockaddr_in));
    dest_in.sin_family = AF_INET;
    // The port is irrelant for Networking and therefore was zeroed.
    dest_in.sin_addr.s_addr = inet_addr(argv[1]);

    //for calculate the time
    struct timeval start, end;
    //seq counter
    int  icmp_seq_counter = 0;
    while (1) {

        // ICMP-header
        struct icmp icmphdr;
        char data[IP_MAXPACKET] = "This is the ping.\n";
        int datalen = strlen(data) + 1;

        icmphdr.icmp_type = ICMP_ECHO;
        // Message Type (8 bits): ICMP_ECHO_REQUEST
        icmphdr.icmp_code = 0;
        // Identifier (16 bits): some number to trace the response.
        // It will be copied to the response packet and used to map response to the request sent earlier.
        // Thus, it serves as a Transaction-ID when we need to make "ping"
        icmphdr.icmp_id = 18;
        icmphdr.icmp_seq = 0;
        // Sequence Number (16 bits): starts at 0
        icmphdr.icmp_cksum = 0;
        // ICMP header checksum (16 bits): set to 0 not to include into checksum calculation
        char packet[IP_MAXPACKET];
        // Combine the packet
        memcpy((packet), &icmphdr, ICMP_HDRLEN); 
        // Next, ICMP header
        memcpy(packet + ICMP_HDRLEN, data, datalen);
        // After ICMP header, add the ICMP data.
        icmphdr.icmp_cksum = calculate_checksum((unsigned short *) (packet),ICMP_HDRLEN + datalen);
        // Calculate the ICMP header checksum
        memcpy((packet), &icmphdr, ICMP_HDRLEN);

        //if NEW_PING receive a stop signal -> break out and finish the program
        int failed = 1;
        int receive = recv(watchdog_socket, &failed, sizeof(int), 0);//receiving from new_ping the sign
        if(receive > 0){
            break;
        }

        //if NEW_PING don't receive a stop signal -> send to watch dog that NEW_PING ready to start sending ping
        int start_p = 282;// Yuval Ben Yaakov in gematria.
        int send_start = send(watchdog_socket, &start_p, sizeof(int), 0);
        if (send_start == -1) printf("send() failed with error code : %d", errno);
        else if (send_start == 0) printf("peer has closed the TCP connection prior to send().\n");
        
        gettimeofday(&start, 0);
        int time_pass =4*(icmp_seq_counter+1);
        //for test - check if the watchdog make the NEW_PING to shut down
        sleep(time_pass);


        // Send the packet using sendto() for sending datagrams.
        int bytes_sent = sendto(sock, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr *) &dest_in, sizeof(dest_in));
        if (bytes_sent == -1) {
            fprintf(stderr, "sendto() failed with error: %d", errno);
            return -1;
        }

        // Get the ping response
        bzero(packet, IP_MAXPACKET);
        socklen_t len = sizeof(dest_in);
        ssize_t bytes_received = -1;
        while ((bytes_received = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *) &dest_in, &len))) {
            if (bytes_received > 0) {
                // Check the IP header
                struct iphdr *iphdr = (struct iphdr *) packet;
                struct icmphdr *icmphdr = (struct icmphdr *) (packet + (iphdr->ihl * 4));
                break;
            }
        }

        gettimeofday(&end, 0);

        //calculate the time
        float milliseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
        printf("%ld bytes from %s: icmp_seq=%d ttl=10 time=%f ms)\n", bytes_received, argv[1], icmp_seq_counter++,
               milliseconds);
    }

    // Close the raw socket descriptor.
    close(sock);
   

    wait(&status);// waiting for child to finish before exiting
    printf("child exit status is: %d\n", status);
    return 0;
}


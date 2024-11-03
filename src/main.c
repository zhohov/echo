#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "8080" // users will be connected to this port
#define BACKLOG 10

/*
 * struct addrinfo {
 *     int              ai_flags;
 *     int              ai_family;
 *     int              ai_socktype;
 *     int              ai_protocol;
 *     socklen_t        ai_addrlen;
 *     struct sockaddr *ai_addr;       // struct sockaddr_in or _in6
 *     char            *ai_canonname;  //
 *     struct addrinfo *ai_next;       // linked list, next node
 * };
 *
 * */

/*
 * in addrinfo pointer to ai_addr must be a sockaddr_* struct
 *
 * struct sockaddr_in; for IPv4
 * struct sockaddr_in6; for IPv6
 *
 * */

/*
 * getaddrinfo() - call to this function to fill addrinfo structure
 *
 * */

int main(int argc, char **argv)
{
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address informatio
    socklen_t addr_size;
    int rv;

    memset(&hints, 0, sizeof hints);  // make the struct is empty
    hints.ai_family = AF_UNSPEC;      // dont't care IPv4 or IPv6 
    hints.ai_socktype = SOCK_STREAM;  // TCP socket
    hints.ai_flags = AI_PASSIVE;      // fill in my IP for me

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) < 0) {
        return -1;
    }

    char ipstr[INET6_ADDRSTRLEN];

    for(p = servinfo; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }

    // Iterate through the linked list of all addrinfo structures and bind to the first one that is available
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        
        /*
         * bind() syscall
         *
         * bind() assingns the address to the socket reffered to by the sockfd
         * this operation also call as "assigning a name to a socket"
         *
         * int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
         *
         * */
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // free the linked-list with addrinfo structures

    /* 
     * listen() syscall
     *
     * listen() marks socket as a socket will be used to accepted incoming connections
     *
     * int listen(int sockfd, int backlog);
     *
     * */
    if (listen(sockfd, BACKLOG) == -1) {
        perror("server: listen");
        return -1;
    }

    printf("server: waiting for connection...\n");

    for (;;) {
        /*
         * accept() syscall
         *
         * accept() It extracts the first connection request 
         * on the queue of pending connections for the listening socket, sockfd, 
         * creates a new connected socket, and returns a new file descriptor 
         * referring to that socket.
         *
         * int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
         *
         * */
        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size); // accept incoming connections

        if (new_fd == -1) {
            perror("server: accept");
            return -1;
        }

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        size_t bytes_received = recv(new_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Received request:\n%s\n", buffer);
        } else if (bytes_received == -1) {
            perror("server: recv");
        }

        /* 
         * send() syscall
         *
         * transmit a message to another socket
         *
         * int send(int sockfd, const void *msg, int len, int flags); 
         *
         * */
        
        char response[2048];
        const char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\nConnection: keep-alive\r\n\r\n%s";
        ssize_t len = snprintf(response, sizeof(response), msg, (size_t)bytes_received, buffer);

        size_t bytes_send = send(new_fd, response, len, 0);
        if (bytes_send == -1) {
            perror("server: send");
        }

        // close accepted sockfd
        close(new_fd);
    }

    return 0; 
}

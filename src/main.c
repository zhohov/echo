#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT "8080" // users will be connected to this port

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
 * */

/*
 * in addrinfo pointer to ai_addr must be a sockaddr_* struct
 *
 * struct sockaddr_in; for IPv4
 * struct sockaddr_in6; for IPv6
 * */

/*
 * getaddrinfo() - call to this function to fill addrinfo structure
 * */

int main(int argc, char **argv)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // may be AF_INET or AF_INET6
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) < 0) {
        return -1;
    }

    // Iterate through the linked list of all addrinfo structures and bind to the first one that is available
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    return 0; 
}

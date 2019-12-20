#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include <stdexcept>

class BasicNtp {
    struct sockaddr_in serv_addr; // Server address data structure.
    struct hostent *server;      // Server data structure.
    int sockfd; 
public:
    BasicNtp(const char *host_name, int portno=123) {
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            throw std::runtime_error("ERROR opening socket");
        if ((server = gethostbyname(host_name)) == NULL)
            throw std::runtime_error("ERROR, no such host");
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect( sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
            throw std::runtime_error("ERROR connecting" );
    }
    int send(ntp_packet *packet) {
        int n;
        n = write(sockfd, (char *)packet, sizeof (ntp_packet));
        if (n < 0)
            throw std::runtime_error("ERROR writing to socket");
        return n;
    }

    int receive(ntp_packet *packet) {
        int n;
        n = read(sockfd, (char *)packet, sizeof (ntp_packet));
        if (n < 0)
            throw std::runtime_error("ERROR reading from socket");
        return n;
    }
};


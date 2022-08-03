//simple tcp stream socket chat client using threads
//compile with "cc client.c -pthread"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT "9034" 
#define MAXDATASIZE 50 // max sendable bytes

int sockfd;  

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//stylize chat in this order "> message" and flush stdout buffer
void push_stdout_buffer() 
{
  printf("%s", "> ");
  fflush(stdout);
}

//send message to server
void *send_message() 
{
    char buf[MAXDATASIZE]; 
    while(1) {
        push_stdout_buffer();
        memset(buf, 0, MAXDATASIZE);
        fgets(buf, MAXDATASIZE, stdin);
        if(strcmp(buf, "exit\n") == 0) {
            printf("disconnecting from server\n");
            fflush(stdout);
            close(sockfd);
            exit(0);
        } else {  
            int sbytes = send(sockfd, buf, MAXDATASIZE, 0);
        }
    }
} 

//receive message handler
void *receive_message() 
{
    char rec[MAXDATASIZE];

    while(1) {
            //receive message from server
            memset(rec, 0, MAXDATASIZE);
            int gybtes = recv(sockfd, rec, MAXDATASIZE, 0);
            if(gybtes < 0) {
                //can handle errors
                continue;
            } else if (gybtes == 0) {
                //socket terminated
                printf("socket receive terminated\n");
                return 0;
            } else {
                printf("%s", rec);
                push_stdout_buffer();
            }
    }
}

int main()
{
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    
    pthread_t t1, t2;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through linked list and connect to first available socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); 

    // run receive and send threads
    if (pthread_create(&t1, NULL, (void *) &receive_message, NULL) != 0) {
        return 1;
    }
    if (pthread_create(&t2, NULL, (void *) &send_message, NULL) != 0) {
        return 2;
    }
    if (pthread_join(t1, NULL) != 0) {
        return 3;
    }
    if (pthread_join(t2, NULL) != 0) {
        return 4;
    }

    return 0;
}
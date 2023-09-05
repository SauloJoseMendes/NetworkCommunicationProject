#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>


#define PASSWORD_LENGTH 30
#define USERNAME_LENGTH 30
#define BUF_SIZE	5120
void erro(char *msg);


struct ip_mreq mreq;
int  * sock;

void killChildProcesses(){
    kill(0, SIGINT);
    close(*sock);
    free(sock);
    exit(0);
}

void timeToDie(){
    if (setsockopt(*sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
        exit(1); }
    close(*sock);
    free(sock);
    exit(0);
}

void workerListenToMulticast(char * s){
    signal(SIGINT, timeToDie);
    printf("\nPROCESSO FILHO CRIADO COM INPUT %s\n",s);
    sock=(int *) malloc(sizeof(int));
    const char *colonPos = strchr(s, ':');
    char server[16];
    char ports[10];
    if (colonPos != NULL) {
        // Copy the characters before the colon to part1
        size_t length1 = colonPos - s;
        strncpy(server, s, length1);
        server[length1] = '\0';

        // Copy the characters after the colon to part2
        size_t length2 = strlen(colonPos + 1);
        strncpy(ports, colonPos + 1, length2);
        ports[length2] = '\0';
    } else {
        // No colon found, set both parts to empty strings
        server[0] = '\0';
        ports[0] = '\0';
    }
    char* group = server;
    int port =(int) strtol(ports,NULL, 10);
    printf("TIME TO LISTEN ON SERVER: %s and PORT: %u\n",server,port);

    if (port <1){
        perror("Invalid port");
        exit(-1);
    }
    // create what looks like an ordinary UDP socket
    //
    *sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        erro("multicast socket");

    // allow multiple sockets to use the same PORT number
    //
    u_int yes = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0)
        erro("Reusing ADDR failed");


    // set up destination address
    //
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(port);

    // bind to receive address
    //
    if (bind(*sock, (struct sockaddr*) &addr, sizeof(addr)) < 0)
        erro("bind multicast");


    // use setsockopt() to request that the kernel join a multicast group
    //

    mreq.imr_multiaddr.s_addr = inet_addr(group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0)
        erro("setsockopt");

    // now just enter a read-print loop
    //
    char msgbuf[BUF_SIZE];
    socklen_t addrlen = sizeof(addr);
    int nbytes;

    while (1) {
         nbytes = recvfrom(*sock,msgbuf,BUF_SIZE,0,(struct sockaddr *) &addr,&addrlen);
        if (nbytes < 0){
            perror("error receiving data");
            exit(-1);
        }
        msgbuf[nbytes] = '\0';
        printf("%s\n",msgbuf);
    }
}
int main(int argc, char *argv[]) {

    if (argc != 3){
        printf("news_client {endereço do servidor} {PORTO_NOTICIAS}\n");
        return -1;
    }
    signal(SIGINT,killChildProcesses);
    struct sockaddr_in addr;
    struct hostent *hostPtr;
    sock = (int *) malloc(sizeof(int));
    char buffer[BUF_SIZE], myInput[BUF_SIZE];
    myInput[0] = '\0';

    if ((hostPtr = gethostbyname(argv[1])) == 0)
        erro("Não consegui obter endereço");

    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *) (hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) strtol(argv[2], NULL, 10));

    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("socket");

    if (connect(*sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        erro("Connect");
    do {
        do {
            buffer[recv(*sock, buffer, BUF_SIZE -1,0)]= '\0';
            printf("%s", buffer);
        } while (strchr(buffer, '\x04') == NULL);
        if (buffer[0] == '@'){
            buffer[strcspn(buffer, "\n")] = '\0';
            if (fork() == 0)
                workerListenToMulticast(&buffer[1]);
        }
        fflush(stdin);
        fgets(myInput, BUF_SIZE, stdin);
        send(*sock, myInput, strlen(myInput), 0);
    } while (strcmp(myInput, "SAIR\n"));

    printf("CLOSING CLIENT PROCESS\n");
    killChildProcesses();
    return 0;
}



void erro(char *msg) {
    perror(msg);
    exit(-1);
}
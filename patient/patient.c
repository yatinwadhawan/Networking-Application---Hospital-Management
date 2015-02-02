//
//  main.c
//  patient
//
//  Created by Yatin Wadhawan on 11/5/14.
//  Copyright (c) 2014 Home. All rights reserved.
//

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<netdb.h>
#include	<sys/types.h>
#include	<netinet/in.h>
#include	<sys/socket.h>
#include	<arpa/inet.h>
#include	<sys/wait.h>
#include    <pthread.h>
#include    <signal.h>
#include <errno.h>
#include "availability.h"
#include <netinet/in.h>

#define SPORT "21247"
#define MAXDATASIZE 100
#define BUFSIZE 1024
#define DOC1PORT 41247
#define DOC2PORT 42247

/****************************************************************************************/
struct file
{
    char patient[25];
    char insurance[25];
};

struct users* loadUsernamePass(char *file);
void *loadUserNameAndStart(void *p1);
void createTCPSocketAuthenticate(char *auth, struct users *pat, char *, int *,int,int *);
void createUDPSocketDoctor(struct users *pat,char doc[], int port, int, char []);
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int socketArr[10], count = 0;
void handler(int);

int main(int argc, const char * argv[])
{
    struct file *p1, *p2;
    int i=0;
    for(i=0;i<10;i++)
        socketArr[i] = 0;
    sigset(SIGINT, handler);
    p1 = (struct file *)malloc(sizeof(struct file));
    p2 = (struct file *)malloc(sizeof(struct file));

    strcpy(p1->patient,argv[1]);
    strcpy(p1->insurance, argv[2]);
    strcpy(p2->patient,argv[3]);
    strcpy(p2->insurance, argv[4]);

    pthread_t patient1, patient2;
    pthread_create(&patient1, NULL, loadUserNameAndStart, (void *)p1);
    pthread_create(&patient2, NULL, loadUserNameAndStart, (void *)p2);
    pthread_join(patient1, 0);
    pthread_join(patient2, 0);
    
    return 0;
}
/****************************************************************************************/


void handler(int signo)
{
    int i=0;
    printf("as");
    for(i=0;i<10;i++)
    {
        if(socketArr[i]!=0)
            close(socketArr[i]);
        else
            break;
    }
}


struct users
{
    char username[20];
    char password[20];
};

struct users* loadUsernamePass(char *file)
{
    
    char line[25];
    struct users *pat =(struct users *)malloc(sizeof(struct users));
    FILE *fp = fopen(file, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Error in opening file\n");
        exit(1);
    }
    int i=0, z=0;
    fgets(line, sizeof(line), fp);

    for(i=0;line[i] != ' ';i++)
    {
        pat->username[z++] = line[i];
    }
    pat->username[z] = '\0';
    i++;
    z=0;
    while(line[i] !='\0')
    {
        pat->password[z++] = line[i];
        i++;
    }
    pat->password[z] = '\0';

    return pat;
}

struct avail* loadAvailabilityList(char *buf)
{
    struct avail* available=NULL;
    int i=0,z=0;
    char func[20];
    while (buf[i] != '\0')
    {
        if(buf[i] == '\n')
        {
            char index;
            char day[10];
            char time[10];
            func[z]='\0';
            sscanf(func, "%c %s %s",&index,day,time);
            available = insertAvailability(available, index - '0', day, time, "", 0);
            z=0;
            func[z] = '\0';
            i++;
        }
        func[z++] = buf[i];
        i++;
    }
    return available;
}

/****************************Create TCP Connections**************************************/

//Taken from beej in order to perform socket programming only
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//Loading username and start the phases.
void *loadUserNameAndStart(void *p1)
{
    char doc[20];
    int doc_port=0;
    pthread_mutex_lock(&m);
    struct file *p = (struct file *) p1;
    
    
    char line[20];
    FILE *fp = fopen(p->insurance, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error in opening file\n");
        exit(1);
    }

    fgets(line, 20, fp);
    
    long i=0;
    int z=0;
    struct users *pat = loadUsernamePass(p->patient);
    char auth[35];
    strcat(auth, "authenticate ");
    i = strlen(auth);
    for(z=0; pat->username[z] != '\0' ;z++)
    {
        auth[i] = pat->username[z];
        i++;
    }
    auth[i++] = ' ';
    for(z=0; pat->password[z] != '\0' ;z++)
    {
        auth[i] = pat->password[z];
        i++;
    }
    int flag=0;
    auth[i] = '\0';
    pthread_mutex_unlock(&m);

    if(strstr(p->patient, "patient1"))
    {
        createTCPSocketAuthenticate(auth,pat,doc,&doc_port,1,&flag);
        if(flag == 1)
            createUDPSocketDoctor(pat, doc, doc_port, 1,line);
    }
    else
    {
        createTCPSocketAuthenticate(auth,pat,doc,&doc_port,2,&flag);
        if(flag == 1)
            createUDPSocketDoctor(pat, doc, doc_port, 2,line);
    }
}

/*The part of code is refered from http://beej.us/guide/bgnet/output/print/bgnet_USLetter.pdf for the purpose of learning socket programming only. The code snippet taken has been modified according to the specifications of the project. It decribes the APIs for creating TCP and UDP packets in more detailed manner. Rest is developed by Yatin.*/

//Creating TCP connection and performing phase 1 and 2
void createTCPSocketAuthenticate(char *auth, struct users *pat,char doc[], int *port,int number,int *flag)
{
    int sock = 0;
    long bytesused;
    char buf[MAXDATASIZE];
    struct addrinfo addr;
    struct addrinfo *addrInfo, *p;
    char s[INET6_ADDRSTRLEN];
    
    struct hostent *ip;
    char buffer[200];
    ip = (struct hostent *) gethostbyname("nunki.usc.edu");
    
    //Taken from beej in order to perform socket programming only
    int status;int rv;
    memset(&addr, 0, sizeof addr);
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo("nunki.usc.edu", SPORT, &addr, &addrInfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = addrInfo; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock);
            perror("client: connect");
            continue;
        }
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);

    
    pthread_mutex_lock(&m);
    socketArr[count++] = sock;
    pthread_mutex_unlock(&m);
    
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    getsockname(sock, (struct sockaddr *)&sin, &len);
    
    
    /*******************************Authentication*************************************/

   
    
    if(number == 1)
        fprintf(stdout, "Phase 1: Patient 1 has TCP port number %d and IP address %s\n",ntohs(sin.sin_port),inet_ntoa(*((struct in_addr *)ip->h_addr)));
    else
        fprintf(stdout, "Phase 1: Patient 2 has TCP port number %d and IP address %s\n",ntohs(sin.sin_port),inet_ntoa(*((struct in_addr *)ip->h_addr)));
    

    if (send(sock, auth, strlen(auth), 0) == -1)
        printf("Error in sending");
    
    if(number == 1)
        fprintf(stdout, "Phase 1: Authentication request from Patient 1 with username %s and password %s has been sent to the Health Center Server.\n",pat->username,pat->password);
    else
        fprintf(stdout, "Phase 1: Authentication request from Patient 2 with username %s and password %s has been sent to the Health Center Server.\n",pat->username,pat->password);

    while (1)
    {
        if ((bytesused = recv(sock, buf, MAXDATASIZE-1, 0)) > 0) {
            buf[bytesused] = '\0';
            if(strcmp(buf, "success")==0)
            {
                *flag = 1;
                break;
            }
            else
            {
                close(sock);
                *flag = 0;
                break;
            }
        }
    }
    
    if(number == 1)
    {
        pthread_mutex_lock(&m);
        fprintf(stdout, "Phase 1: Patient 1 authentication result: %s.\n",buf);
        fprintf(stdout, "End of Phase 1 for Patient 1.\n");
        pthread_mutex_unlock(&m);
    }
    else
    {
        pthread_mutex_lock(&m);
        fprintf(stdout, "Phase 1: Patient 2 authentication result: %s.\n",buf);
        fprintf(stdout, "End of Phase 1 for Patient 2.\n");
        pthread_mutex_unlock(&m);
    }
    
    
    /*******************************Sending Choice of Timing*************************************/
    if(*flag == 1)
    {
        sleep(1);
        if (send(sock, "available", strlen("available"), 0) == -1)
            printf("Error in sending");

        while (1)
        {
            if ((bytesused = recv(sock, buf, MAXDATASIZE-1, 0)) > 0)
            {
                buf[bytesused] = '\0';
                break;
            }
        }
        
        pthread_mutex_lock(&m);
        
        if(strstr(buf,"no"))
        {
            close(sock);
            fprintf(stdout,"Not Available Time slots.\n");
            exit(0);
        }
        
        if(number == 1)
        {
            fprintf(stdout, "Phase 2: The following appointments are available for Patient 1:\n");
            fprintf(stdout, "\n%s\n",buf);
        }
        else
        {
            fprintf(stdout, "Phase 2: The following appointments are available for Patient 2:\n");
            fprintf(stdout, "\n%s\n",buf);
        }
        
        struct avail *available = loadAvailabilityList(buf);

        int c = 0;
        while(1)
        {
            fprintf(stdout, "Please enter the preferred appointment index and press enter: \n");
            scanf("%d",&c);
            if(isAvailable(available, c))
            {
                break;
            }
        }
        
        char select[20] = "selection ";
        char c1[3];
        memset(c1, 0, sizeof(c1));
        c1[0] = (char)(((int)'0')+c);
        c1[1] = '\0';
        strcat(select, c1);
        
        if (send(sock, select, strlen(select), 0) == -1)
            printf("Error in sending");
        

        buf[0] = '\0';
        while (1)
        {
            if ((bytesused = recv(sock, buf, MAXDATASIZE-1, 0)) > 0)
            {
                buf[bytesused] = '\0';
                break;
            }
        }
        if(strstr(buf, "notavailable"))
        {
            *flag = 0;
            if(number == 1)
            {
                fprintf(stdout, "Phase 2: The requested appointment from patient 1 is not available. Exiting...\n");
            }
            else
            {
                fprintf(stdout, "Phase 2: The requested appointment from patient 2 is not available. Exiting...\n");
            }
        }
        else
        {
            *flag = 1;
            sscanf(buf, "%s %d",doc,port);
            *port = *port + 247;
        
            sleep(1);
            if(number == 1)
                fprintf(stdout, "Phase 2: The requested appointment is available and reserved to Patient 1. The assigned doctor port number is %d\n",(*port));
            else
                fprintf(stdout, "Phase 2: The requested appointment is available and reserved to Patient 2. The assigned doctor port number is %d\n",(*port));
            
        }
        pthread_mutex_unlock(&m);

    }
    /********************************************************************/

    close(sock);
}

//creating UDP connection to the server and performing phase 3
void createUDPSocketDoctor(struct users *pat,char doc[], int port,int number,char insurance[])
{
    int sockfd,n;
    struct sockaddr_in servaddr,cliaddr;
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr("nunki.usc.edu");
    servaddr.sin_port=htons(port);
    
    struct hostent *ip;
    char buffer[BUFSIZE];
    ip = (struct hostent *) gethostbyname("nunki.usc.edu");
    
    sleep(1);
    if(sendto(sockfd,insurance,strlen(insurance),0,(struct sockaddr *)&servaddr,sizeof(servaddr)) == -1)
        fprintf(stdout, "Error in sending insurance to doctor");
    
    socklen_t len = sizeof(cliaddr);
    getsockname(sockfd, (struct sockaddr *)&cliaddr, &len);

    if(number == 1)
        fprintf(stdout, "Phase 3: Patient 1 has a dynamic UDP port number %d and IP address %s.\n",ntohs(cliaddr.sin_port), inet_ntoa(*((struct in_addr *)ip->h_addr)));
    else
        fprintf(stdout, "Phase 3: Patient 2 has a dynamic UDP port number %d and IP address %s.\n",ntohs(cliaddr.sin_port), inet_ntoa(*((struct in_addr *)ip->h_addr)));
    

    
    if(number == 1)
    {
        fprintf(stdout, "Phase 3: The cost estimation request from Patient 1 with insurance plan %s has been sent to the doctor with port number %d and IP address %s.\n",insurance, port, inet_ntoa(*((struct in_addr *)ip->h_addr)));
    }
    else
    {
        fprintf(stdout, "Phase 3: The cost estimation request from Patient 2 with insurance plan %s has been sent to the doctor with port number %d and IP address %s.\n",insurance, port, inet_ntoa(*((struct in_addr *)ip->h_addr)));
    }
    
    while (1)
    {
        size_t len = recvfrom(sockfd,buffer,BUFSIZE,0,NULL,NULL);
        if (len > 0)
        {
            buffer[len] = '\0';
            break;
        }
    }
    if(strstr(doc,"1"))
        doc = "Doctor 1";
    else
        doc = "Doctor 2";
    
    sleep(1);
    if(number == 1)
    {
        fprintf(stdout, "Phase 3: Patient 1 receives %s$ estimation cost from doctor with port number %d amd name %s\n",buffer,port,doc);
        fprintf(stdout, "Phase 3: End of Phase 3 for Patient 1.\n");
    }
    else
    {
        fprintf(stdout, "Phase 3: Patient 2 receives %s$ estimation cost from doctor with port number %d amd name %s\n",buffer,port,doc);
        fprintf(stdout, "Phase 3: End of Phase 3 for Patient 2.\n");
    }
    
    close(sockfd);

}

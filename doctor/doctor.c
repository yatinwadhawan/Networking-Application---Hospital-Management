//
//  main.c
//  Doctor
//
//  Created by Yatin Wadhawan on 11/15/14.
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

#define DOC1PORT "41247"
#define DOC2PORT "42247"
#define DOC1_PORT 41247
#define DOC2_PORT 42247

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
size_t BUFSIZE = 2048;
int sockArr[2], count=0;
void handle(int);

/************************************************************************************************/

struct insurance
{
    char name[20];
    long price;
    struct insurance *next;
};

int port;

//Inserting the insurance list doctor serve
struct insurance* insertInsurance(struct insurance *insu, char name[], long price)
{
    struct insurance *temp = (struct insurance *)malloc(sizeof(struct insurance));
    strncpy(temp->name, name, strlen(name));
    temp->price = price;
    
    temp->next = insu;
    insu = temp;
    return  insu;
}

void display(struct insurance *insu)
{
    struct insurance *temp = insu;
    while (temp != NULL)
    {
        fprintf(stdout, "%s %lu \n",temp->name,temp->price);
        temp = temp->next;
    }
}

long getPriceOfInsurance(struct insurance *insu, char name[])
{
    struct insurance *temp = insu;
    while (temp != NULL)
    {
        if(strcmp(temp->name, name) == 0)
            return temp->price;
        temp = temp->next;
    }
    return -1;
}

struct insurance * loadInsuranceData(struct insurance *insu, char *file)
{
    char line[100];
//    char path[100] = "/Users/yatinwadhawan/Documents/Computer Networks/Doctor/Doctor/";
//    char *path1 = strcat(path, file);
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error in opening file\n");
        exit(1);
    }
    while (fgets(line, sizeof(line), fp))
    {
        char *name = strtok(line, " ");
        char *price = strtok(NULL, " ");
        insu = insertInsurance(insu, name, strtol(price, NULL, 10));
    }
    
    return insu;

}
/************************************************************************************************/

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*The part of code is refered from http://beej.us/guide/bgnet/output/print/bgnet_USLetter.pdf for the purpose of learning socket programming only. The code snippet taken has been modified according to the specifications of the project. It decribes the APIs for creating TCP and UDP packets in more detailed manner. Rest is developed by Yatin.*/

//creating UDP connection and listening to the patients.
void *startListeningtoUDP (void *file)
{
    int sockfd,n;
    struct sockaddr_in servaddr,cliaddr;
    socklen_t len;
    char mesg[BUFSIZE];
    char docport[10];
    
    struct insurance *insu = NULL;
    pthread_mutex_lock(&m);
    insu = loadInsuranceData(insu, file);
    pthread_mutex_unlock(&m);
    
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(strstr(file,"1"))
    {
        servaddr.sin_port=htons(DOC1_PORT);
        strcpy(docport,DOC1PORT);
    }
    else
    {
        servaddr.sin_port=htons(DOC2_PORT);
        strcpy(docport,DOC2PORT);
    }
    struct hostent *ip;
    char buffer[BUFSIZE];
    gethostname(buffer, BUFSIZE);
    ip = (struct hostent *) gethostbyname(buffer);
    bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    
    len = sizeof(cliaddr);
    
    if(strcmp(docport, DOC1PORT) == 0)
        fprintf(stdout, "Phase 3: Doctor 1 has a static UDP port %s and IP address %s.\n",DOC1PORT,inet_ntoa(*((struct in_addr *)ip->h_addr)));
    else
        fprintf(stdout, "Phase 3: Doctor 2 has a static UDP port %s and IP address %s.\n",DOC2PORT,inet_ntoa(*((struct in_addr *)ip->h_addr)));
    
    
    while(1)
    {
        while (1)
        {
            size_t l = recvfrom(sockfd,mesg,BUFSIZE,0,(struct sockaddr *)&cliaddr,&len);
            if (l > 0)
            {
                mesg[l] = '\0';
                break;
            }
        }
        
        sleep(1);
        if(strcmp(docport, DOC1PORT) == 0)
            fprintf(stdout, "Phase 3: Doctor 1 receives the request from the patient with the port number %d and name  with the insurance plan %s\n",ntohs(cliaddr.sin_port), mesg);
        else
            fprintf(stdout, "Phase 3: Doctor 2 receives the request from the patient with the port number %d and name  with the insurance plan %s\n",ntohs(cliaddr.sin_port),mesg);
        
        long pr = getPriceOfInsurance(insu, mesg);
        char price[10];
        sprintf(price, "%lu",pr);
    
        sleep(1);
        if (sendto(sockfd,price,strlen(price),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr)) == -1)
            fprintf(stdout, "Error in sending price to patient\n");
    
        if(strcmp(docport, DOC1PORT) == 0)
            fprintf(stdout, "Phase 3: Doctor 1 has sent estimated price %s$ to patient with port number %d \n",price,ntohs(cliaddr.sin_port));
        else
            fprintf(stdout, "Phase 3: Doctor 2 has sent estimated price %s$ to patient with port number %d \n",price,ntohs(cliaddr.sin_port));
    }
}


void handle(int signal)
{
    close(41245);
    close(42245);
    close(sockArr[0]);
    close(sockArr[1]);
}

int main(int argc, const char * argv[])
{
    pthread_t doc1, doc2;
    int err = pthread_create(&doc1, NULL, startListeningtoUDP, (void *)argv[1]);
    err = pthread_create(&doc2, NULL, startListeningtoUDP, (void *)argv[2]);
    pthread_join(doc1, (void *)0);
    pthread_join(doc2, (void *)0);
    return 0;
}


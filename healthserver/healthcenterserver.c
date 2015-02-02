//
//  main.c
//  EE450
//
//  Created by Yatin Wadhawan on 11/4/14.
//  Copyright (c) 2014 Home. All rights reserved.
//  Healthcare Server File

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
#include <signal.h>

#include "availabilityserver.h"
#define SPORT "21247"
#define MAXDATASIZE 100



/****************************************************************************************/
/****************************************************************************************/
//Data Structure to Store list of users in the users.txt file.
//I have used LinkedList to store all the users, having their usernames and passwords.
struct users
{
    char username[20];
    char password[20];
    struct users *next;
};

int x=0;

struct users *user=NULL;

//Inserting all the users in the linkedlist
void insertUser(char username[], char password[])
{
    struct users *temp = (struct users *)malloc(sizeof(struct users));
    int i=0;
    while(username[i]!='\0' && username[i]!='\n')
    {
        temp->username[i] = username[i];
        i++;
    }
    temp->username[i]='\0';
    
    i=0;
    while(password[i]!='\0' && password[i]!='\n')
    {
        temp->password[i] = password[i];
        i++;
    }
    temp->password[i]='\0';
    temp->next = user;
    user = temp;
}

//For finding whether username and password present in the database.
int isPresent(char username[], char password[])
{
    struct users *temp = user;
    while(temp!=NULL)
    {
        if(strcmp(temp->username, username) == 0 && strcmp(temp->password, password) == 0)
            return 1;
        temp = temp->next;
    }
    return 0;
}

//Displaying all the users in the file
void displayUsersLinkedList()
{
    struct users *temp = user;
    while(temp != NULL)
    {
        printf("username : %s password : %s\n",temp->username,temp->password);
        temp = temp->next;
    }
}

//Loading the users from the file.
void loadUsersDataFromFile(const char *file)
{
    char line[100];
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error in opening file\n");
        exit(1);
    }
    while (fgets(line, sizeof(line), fp))
    {
        char *user = strtok(line, " ");
        char *pass = strtok(NULL, " ");
        insertUser(user, pass);
    }
}
/****************************************************************************************/
/****************************************************************************************/

//Data Structure for Doctor's appointment list used from Availability.h
//struct avail *available = NULL;

//Loading the list from the file using fscanf function.
void loadAvailabilityList(const char *file)
{
    int index;
    char day[10];
    char time[10];
    char docid[10];
    int port=0;
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error in opening file\n");
        exit(1);
    }
    while (fscanf(fp, "%d %s %s %s %d",&index,day,time,docid,&port) != EOF)
    {
       insertAvailability(index, day, time, docid, port);
    }
}

//Global Variables

int sock = 0;
// Retreiving Socket Address IPv4
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* authentication(char[], int,int *);
void proccessing(int);
int sendTimeSlots(int,char *);

pthread_mutex_t m1 = PTHREAD_MUTEX_INITIALIZER;

/*The part of code is refered from http://beej.us/guide/bgnet/output/print/bgnet_USLetter.pdf for the purpose of learning socket programming only. The code snippet taken has been modified according to the specifications of the project. It decribes the APIs for creating TCP and UDP packets in more detailed manner. Rest is developed by Yatin.*/

int main(int argc, const char * argv[])
{
    int status,allow=1;
    struct addrinfo addr;
    socklen_t addr_size;
    struct sockaddr_storage their_addr;
    struct addrinfo *addrInfo;
    void handler(int);
    
    memset(&addr, 0, sizeof addr);
    addr.ai_family = AF_UNSPEC;
    addr.ai_socktype = SOCK_STREAM; // TCP sockets
    addr.ai_flags = AI_PASSIVE;
    status = getaddrinfo(NULL, SPORT , &addr, &addrInfo);
    
    loadAvailabilityList(argv[1]);
    loadUsersDataFromFile(argv[2]);
    reverse();
    
    sigset(SIGINT, handler);
    

    if (status != 0)
    {
        fprintf(stderr, "main() : getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
    sock = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol); //Creating the socket.
    if(sock == -1)
    {
        fprintf(stderr, "main() : Error in creating socket\n");
        exit(1);
    }
    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &allow,sizeof(int)) == -1)
    {
        fprintf(stderr, "main() : setSockOpt error\n");
        exit(1);
    }
    
    int ret = bind(sock, addrInfo->ai_addr, addrInfo->ai_addrlen);
    if(ret == -1)
    {
        fprintf(stderr, "main() : Error in binding port to the socket\n");
        exit(1);
    }
    
    
    ret = listen(sock, 5);
    if(ret == -1)
    {
        fprintf(stderr, "main() : Error in Listining to socket\n");
        exit(1);
    }
    
    struct hostent *ip;
    char buffer[200];
    ip = (struct hostent *) gethostbyname("nunki.usc.edu");
    

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(sock, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
    {
        printf("Phase 1: The Health Center Server has port number %s and IP	address	%s\n",SPORT,inet_ntoa(*((struct in_addr *)ip->h_addr)));
    }

    while(1)
    {
        addr_size = sizeof their_addr;
        int newSockId = accept(sock, (struct sockaddr *)&their_addr, &addr_size);
        if(newSockId < 1)
        {
            printf("main() : Error in accepting clients");
            exit(1);
        }
        
        int pid = fork();
        
        if (pid < 0)
        {
            perror("ERROR on fork");
            exit(1);
        }
        if (pid == 0)
        {
            proccessing(newSockId);
        }
        else
        {
            close(newSockId);
        }
    }
}

//signalling handler
void handler(int signo)
{
    close(sock);
}

//processing of each thread.
void proccessing(int newSockId)
{
    long bytesused=0;
    char buf[MAXDATASIZE];
    char *user = NULL;
    while(1)
    {
        if ((bytesused = recv(newSockId, buf, MAXDATASIZE-1, 0)) > 0)
        {
            buf[bytesused] = '\0';
            break;
        }
    }
    int flag=0;
    if(strstr(buf, "authenticate"))
    {
        pthread_mutex_lock(&m1);
        user = authentication(buf,newSockId,&flag);
        pthread_mutex_unlock(&m1);
    }
        while(1)
        {
            if ((bytesused = recv(newSockId, buf, MAXDATASIZE-1, 0)) > 0)
            {
                buf[bytesused] = '\0';
                break;
            }
        }
        
        if(strstr(buf, "available"))
        {
            struct hostent *ip;
            char buffer[200];
            gethostname(buffer, 200);
            ip = (struct hostent *) gethostbyname(buffer);
            
            socklen_t len;
            struct sockaddr_storage cliaddr;
            char ipstr[INET6_ADDRSTRLEN];
            int port;
            
            len = sizeof cliaddr;
            getpeername(newSockId, (struct sockaddr*)&cliaddr, &len);
            
            struct sockaddr_in *s = (struct sockaddr_in *)&cliaddr;
                
            fprintf(stdout,"Phase 2: The Health Center Server receives a request for available timeslots from patients with port %d and IP address %s \n",ntohs(s->sin_port),inet_ntoa(*((struct in_addr *)ip->h_addr)));
            sendTimeSlots(newSockId,user);
        }
    close(newSockId);
}

//sending timeslots to the health server
int sendTimeSlots(int newSockId, char *user)
{
    char *data = getAvailableList();
    if(strlen(data) == 0)
        strcpy(data, "no");
    if (send(newSockId, data, strlen(data), 0) == -1)
         fprintf(stderr,"Error in sending\n");
    
    fprintf(stdout,"Phase 2: The Health Center Server sends available time slots to patient with username %s\n",user);

    long bytesused;
    char buf[MAXDATASIZE];

    while (1)
    {
        if ((bytesused = recv(newSockId, buf, MAXDATASIZE-1, 0)) > 0) {
            buf[bytesused] = '\0';
            break;
        }
    }

    int i=0;
    char c[2];
    while(buf[i] != ' ')
        i++;
    c[0] = buf[++i];c[1]='\0';
    int index = atoi(c);

    
    pthread_mutex_lock(&m1);
    
    socklen_t len;
    struct sockaddr_storage cliaddr;
    char ipstr[INET6_ADDRSTRLEN];
    int port;
    
    len = sizeof cliaddr;
    getpeername(newSockId, (struct sockaddr*)&cliaddr, &len);
    struct sockaddr_in *s = (struct sockaddr_in *)&cliaddr;
    
    fprintf(stdout,"Phase 2: The Health Center Server receives a request for appointment from patient with port number %d and username %s\n",ntohs(s->sin_port), user);

    if(isAvailable(index) == 1)
    {
        reserverDoc(index);
        struct avail *temp = getListItem(index);
        char doc[30],in[6];
        strcpy(doc, temp->docid);
        strcat(doc, " ");
        sprintf(in, "%d", temp->port);
        strcat(doc, in);
        
        fprintf(stdout,"Phase 2: The Health Center Server confirms the following appointment %d to patient with username %s\n",index, user);
        sleep(1);
        
        if (send(newSockId, doc, strlen(doc), 0) == -1)
            fprintf(stderr,"Error in sending\n");
        pthread_mutex_unlock(&m1);
        return 1;
    }
    else
    {
        fprintf(stdout,"Phase 2: The Health Center Server rejects the following appointment %d to patient with username %s\n",index, user);
        sleep(1);
        
        if (send(newSockId, "notavailable", strlen("snotavailable"), 0) == -1)
            fprintf(stderr,"Error in sending\n");
        pthread_mutex_unlock(&m1);
        close(newSockId);
        return  0;
        
    }
}

//Authentication for patients
char* authentication(char buf[], int newSockId,int *flag)
{
    int i=0,y=0,z=0,s=0;
    unsigned long len = strlen(buf);
    char *user = (char *)malloc(20*sizeof(char)),pass[20];
    while (i<len)
    {
            if(s == 1 && buf[i]!=' ')
            {
                user[y++] = buf[i];
            }
            else if(s == 2 && buf[i]!=' ')
            {
                pass[z++] = buf[i];
            }
            if(buf[i] == ' ')
                s++;
            i++;
    }
    user[y] = '\0';
    pass[z] = '\0';
    
    fprintf(stdout,"Phase 1: The Health Center Server has reveived request from a patient with username %s and password %s.\n",user,pass);

    if(isPresent(user, pass))
    {
        if (send(newSockId, "success", 13, 0) == -1)
            fprintf(stderr,"Error in sending\n");
        fprintf(stdout, "Phase 1: The Health Center Server sends the response success to the patient with username %s.\n",user);
        *flag = 1;
        return user;
    }
    else
    {
        if (send(newSockId, "failure", 13, 0) == -1)
            fprintf(stderr,"Error in sending\n");
        fprintf(stdout, "Phase 1: The Health Center Server sends the response failure to the patient with username %s.\n",user);
        *flag = 0;
        return user;
    }
}


//
//  Availability.h
//  EE450
//
//  Created by Yatin Wadhawan on 11/5/14.
//  Copyright (c) 2014 Home. All rights reserved.
//

//In this code, I have used my own logic of creating data structute to store and I have refererd to http://www.makelinux.net/alp/035 for
//learning that how to share a memory when fork is used. Since fork creates the copy of whole process address, it is important to create a
//shared memory.

#ifndef EE450_Availability_h
#define EE450_Availability_h
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
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



//Data Structure for Doctor's appointment list

struct avail
{
    int index;
    char day[10];
    char time[10];
    char docid[10];
    int port;
    int isAvailable;
    struct avail *next;
};

struct avail *available = NULL;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;



//Inserting all the users in the linkedlist
void insertAvailability(int index, char day[], char time[], char docid[], int port)
{
    pthread_mutex_lock(&m);
    struct avail *temp  = (struct avail *) mmap(NULL, sizeof *temp, PROT_READ | PROT_WRITE,MAP_SHARED|MAP_ANON, -1, 0);
  

    temp->index = index;
    int i=0;
    while(day[i]!='\0' && day[i]!='\n')
    {
        temp->day[i] = day[i];
        i++;
    }
    temp->day[i]='\0';
    i=0;
    while(time[i]!='\0' && time[i]!='\n')
    {
        temp->time[i] = time[i];
        i++;
    }
    temp->time[i]='\0';
    i=0;
    while(docid[i]!='\0' && docid[i]!='\n')
    {
        temp->docid[i] = docid[i];
        i++;
    }
    temp->docid[i]='\0';
    temp->port = port;
    temp->isAvailable = 1;
    
    temp->next = available;
    available = temp;
    pthread_mutex_unlock(&m);

}

//Displaying the availability list
void displayAvailableList()
{
    pthread_mutex_lock(&m);

    struct avail *temp = available;
    while(temp != NULL)
    {
        printf("%d %s %s %s %d %d\n",temp->index,temp->time,temp->day,temp->docid,temp->port,temp->isAvailable);
        temp = temp->next;
    }
    pthread_mutex_unlock(&m);

}

//check if a particular index is available
int isAvailable(int index)
{
    pthread_mutex_lock(&m);

    struct avail *temp = available;
    while(temp != NULL)
    {
        if(temp->index == index)
        {
            if(temp->isAvailable == 1)
            {
                pthread_mutex_unlock(&m);
                return 1;
            }
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&m);
    return 0;
}

struct avail* getListItem(int index)
{
    pthread_mutex_lock(&m);

    struct avail *temp = available;
    while(temp != NULL)
    {
        if(temp->index == index)
        {
            pthread_mutex_unlock(&m);
            return temp;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&m);

    return NULL;
}

void reserverDoc(int index)
{
    pthread_mutex_lock(&m);

    struct avail *temp = available;
    while(temp != NULL)
    {
        if(temp->index == index)
        {
            temp->isAvailable = 0;
            break;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&m);
}

void reverse()
{
    pthread_mutex_lock(&m);

    struct avail *prev= NULL, *next, *temp = available;
    while(temp != NULL)
    {
        next = temp->next;
        temp->next = prev;
        prev = temp;
        temp = next;
    }
    temp = prev;
    available = prev;
    pthread_mutex_unlock(&m);

}

char* getAvailableList()
{
    pthread_mutex_lock(&m);

    char *list = (char *)malloc(1024*sizeof(char));
    int z=0,i=0;
    struct avail* temp = available;
    while(temp != NULL)
    {
        if(temp->isAvailable == 1)
        {
            list[z++] = (char)(((int)'0')+temp->index);
            list[z++] = ' ';
            for(i=0;temp->day[i]!='\0';i++)
                list[z++] = temp->day[i];
            list[z++] = ' ';
            for(i=0;temp->time[i]!='\0';i++)
                list[z++] = temp->time[i];
            list[z++] = '\n';
        }
        temp = temp->next;
    }
    list[z] = '\0';
    pthread_mutex_unlock(&m);

    return list;
}



#endif

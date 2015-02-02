//
//  Availability.h
//  patient
//
//  Created by Yatin Wadhawan on 11/5/14.
//  Copyright (c) 2014 Home. All rights reserved.
//

#ifndef patient_Availability_h
#define patient_Availability_h

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

//Inserting all the users in the linkedlist
struct avail* insertAvailability(struct avail *available,int index, char day[], char time[], char docid[], int port)
{
    struct avail *temp = (struct avail *)malloc(sizeof(struct avail));
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
    return available;
}

//Displaying the availability list
void displayAvailableList(struct avail *available)
{
    struct avail *temp = available;
    while(temp != NULL)
    {
        printf("%d %s %s %s %d %d\n",temp->index,temp->time,temp->day,temp->docid,temp->port,temp->isAvailable);
        temp = temp->next;
    }
}

//check if a particular index is available
int isAvailable(struct avail *available, int index)
{
    struct avail *temp = available;
    while(temp != NULL)
    {
        if(temp->index == index)
        {
            if(temp->isAvailable)
                return 1;
        }
        temp = temp->next;
    }
    return 0;
}





#endif

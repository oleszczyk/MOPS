/*
 * local_broker.c
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include "MOPS.h"
#include "MOPS_RTnet_Con.h"


static uint8_t MOPS_State = SEND_REQUEST;
uint8_t input_buffer[UDP_MAX_SIZE], output_buffer[UDP_MAX_SIZE];
uint16_t writtenBytes = 0, output_index = 0;
TopicID list[MAX_NUMBER_OF_TOPIC];


#if TARGET_DEVICE == Linux
pthread_mutex_t output_lock, input_lock;
#endif
#if TARGET_DEVICE == RTnode
SemaphoreHandle_t output_lock, input_lock;
#endif


int main(void)
{
	int RTsocket = 0;
	mutex_init(&input_lock);
	mutex_init(&output_lock);

	InitTopicList(list);
	AddTopicToList(list, "Rudy", 4, 2);
	AddTopicToList(list, "Michal", 6, 128);
	AddTopicCandidate("Krowa", 5);
	AddTopicToList(list, "GGG", 3, 257);
	//PrintfList(list);
	AddTopicToList(list, "Krowa", 5, 3);

	RTsocket = connectToRTnet();

    startNewThread(&threadAction, (void *)RTsocket);

    for(;;){
		receiveFromRTnet(RTsocket, input_buffer, UDP_MAX_SIZE);
		AnalizeIncomingUDP(input_buffer, UDP_MAX_SIZE);
    }
}

void threadAction(int RTsocket){
	for(;;){
		sleep(2);  // slot czasowy
		lock_mutex(&output_lock);

		switch(MOPS_State){
		case SEND_NOTHING:
			output_index += buildEmptyMessage(output_buffer, UDP_MAX_SIZE);
			break;
		case SEND_REQUEST:
			output_index += buildTopicRequestMessage(output_buffer, UDP_MAX_SIZE);
			break;
		case SEND_TOPIC_LIST:
			output_index += SendTopicList(output_buffer, UDP_MAX_SIZE, list);
			break;
		case SEND_NEW_TOPIC:
			//check if topic you want to announce is not already in your TopicList
			//if not then add it to head of message and update TopicList
			//else, send 'nothing'
			break;
		}

		if (output_index > 0){
			sendToRTnet(RTsocket, output_buffer, output_index);
			MOPS_State = SEND_NOTHING;
			memset(output_buffer, 0, UDP_MAX_SIZE);
			output_index = 0;
		}
		unlock_mutex(&output_lock);
	}
}

uint16_t SendTopicList(uint8_t *Buffer, int BufferLen, TopicID list[]){
	int i = 0, counter = 0;
	uint8_t *tempTopicList[MAX_NUMBER_OF_TOPIC];
	uint16_t tempTopicIDs[MAX_NUMBER_OF_TOPIC];
	uint16_t writtenBytes;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (list[i].ID != 0){
			tempTopicList[counter] = &list[i].Topic;
			tempTopicIDs[counter] = list[i].ID;
			counter++;
		}
	}

	writtenBytes = buildNewTopicMessage(Buffer, BufferLen, tempTopicList, tempTopicIDs, counter);
	return writtenBytes;
}

uint8_t AddTopicToList(TopicID list[], uint8_t *topic, uint16_t topicLen, uint16_t id){
	int i = 0;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		//if candidate, apply ID
		if( strncmp(list[i].Topic, topic, topicLen)==0 && list[i].Topic[0]!=0 && list[i].ID==0 ){
			list[i].ID = id;
			printf("Dodalem ID kandydatowi: %s \n", list[i].Topic);
			return 0;
		}
		// if exists such topic (or at least ID) available, do not do anything
		if ( (list[i].ID == id) || (strncmp(list[i].Topic, topic, topicLen)==0 && list[i].Topic[0]!=0) ){
			printf("Nie dodam bo jest: %s \n", list[i].Topic);
			return 2;
		}
	}

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		//else add new topic in the first empty place
		if ( list[i].ID==0 && strlen(list[i].Topic)==0 ){
			memcpy(list[i].Topic, topic, topicLen);
			printf("Dodany: %s \n", list[i].Topic);
			list[i].ID = id;
			return 0;
		}
	}
	//there is no place in TopicList
	return 1;
}


void AddTopicCandidate(uint8_t *topic, uint16_t topicLen){
	int i;
	if(GetIDfromTopicName(topic, topicLen) == -1)
		for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
			if ( list[i].ID==0 && strlen(list[i].Topic)==0 ){
				memcpy(list[i].Topic, topic, topicLen);
				return;
			}
		}
}

/*
 * return:
 *  ID (uint16_t value) if topic exist already in TopicList and is available
 *  0					if topic is candidate in TopicList
 *  -1					if topic is not available, and not candidate
 */
int GetIDfromTopicName(uint8_t *topic, uint16_t topicLen){
	int i;
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (strncmp(list[i].Topic, topic, topicLen)==0 && list[i].Topic[0]!=0)  //when  are the same
				return list[i].ID;
	}
	return -1;
}

/*
 * POST: variable 'topic' is set as Topic with id 'id',
 * if there is not a topic in TopicList with that id
 * variable 'topic' is set to \0.
 */
void GetTopicNameFromID(uint16_t id, uint8_t *topic, uint16_t topicLen){
	int i;
	memset(topic, 0, topicLen);
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (list[i].ID == id)  //when  are the same
			memcpy(topic, &list[i].Topic, topicLen);
	}
}

void InitTopicList(TopicID list[]){
	int i = 0;
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		list[i].ID = 0;
		memset(&list[i].Topic, 0, MAX_TOPIC_LENGTH+1);
	}
}

void PrintfList(TopicID list[]){
	int i;
	printf("Lista{\n");
	for(i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		printf("    Topic: %s, ID: %d \n", list[i].Topic, list[i].ID);
	}
	printf("};\n");
}

void AddClientIDToPacket(uint8_t *buf, uint8_t ClientID, int *WrittenBytes, int nbytes){
	memmove(buf + sizeof(ClientID), buf, nbytes);
	memcpy(buf, &ClientID, sizeof(ClientID));
	(*WrittenBytes) += sizeof(ClientID);
}


void AnalizeIncomingUDP(uint8_t *Buffer, uint8_t BufferLen){
	MOPSHeader MHeader;
	memcpy(&MHeader, Buffer, sizeof(MHeader));

	switch(MHeader.MOPSMessageType){
	case TOPIC_REQUEST:
		lock_mutex(&output_lock);
		MOPS_State = SEND_TOPIC_LIST;
		unlock_mutex(&output_lock);
		break;
	case NEW_TOPICS:
		lock_mutex(&output_lock);
		UpdateTopicList(Buffer, BufferLen);
		unlock_mutex(&output_lock);
		break;
	case NOTHING:
		//do not change state
		break;
	}
}

void UpdateTopicList(uint8_t *Buffer, uint8_t BufferLen){
	uint16_t index = 0, messageLength = 0;
	uint16_t tempTopicLength = 0, tempTopicID = 0;
	uint8_t tempTopic[MAX_TOPIC_LENGTH], err;

	messageLength = MSBandLSBTou16(Buffer[1], Buffer[2]) + 3;
	index += 3;
	for(; index<messageLength; ){
		tempTopicID = MSBandLSBTou16(Buffer[index], Buffer[index+1]);
		tempTopicLength = MSBandLSBTou16(Buffer[index+2], Buffer[index+3]);
		index += 4;
		memcpy(tempTopic, Buffer+index, tempTopicLength);
		err = AddTopicToList(list, tempTopic, tempTopicLength, tempTopicID);
		index += tempTopicLength;
		if(err == 1)
			printf("Brak miejsca na liscie! \n");
		if(err == 0)
			printf("Dodalem, id: %d \n", tempTopicID);
		if(err == 2)
			printf("Topic, id: %d, juz istnieje. \n", tempTopicID);
	}
}
/*
 * int main(void)
{
	struct timeval tv;
    int t, len, i, rv, nbytes;
    struct sockaddr_un local, remote;
    int inputWrittenIndex = 0, outpuWrittenIndex = 0;
    int free_space;
    int licznik = 0;
    fd_set master;  //master fd list
    fd_set read_fd; //temp fd list for select()
	int fdmax;		//maximum fd number
	int listener;   //listening socket descriptor
	int newfd;		//newly accept()ed socket descriptor
	memset(input_buffer, 0, sizeof(input_buffer));
	memset(output_buffer, 0, sizeof(output_buffer));
    if ((listener = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(listener, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(listener, 5) == -1) {
        perror("listen");
        exit(1);
    }
	FD_ZERO(&master);
	FD_ZERO(&read_fd);
    //add listener to the master set
    FD_SET(listener, &master);
    fdmax = listener;
    for(;;){
    	free_space = sizeof(input_buffer)-(inputWrittenIndex*sizeof(input_buffer[0]));
    	tv.tv_sec = 0;
        tv.tv_usec = 10000;
    	read_fd = master;
    	if( free_space <= sizeof(input_buffer)/10 )
    		rv = 0;		//go to "idle state"
		else{
			if((rv = select(fdmax+1, &read_fd, NULL, NULL, &tv)) == -1){
				perror("select");
				exit(4);
			}
		}
    	if(rv > 0){
			for(i = 0; i <=fdmax; i++){
				if (FD_ISSET(i, &read_fd)){
					if(i == listener){
						t = sizeof(remote);
						newfd = accept(listener,(struct sockaddr *)&remote, &t);
						if(newfd == -1){
							perror("accept");
						}
						else{
							FD_SET(newfd, &master);
							if(newfd > fdmax)
								fdmax = newfd;
							printf("Nowy deskryptor: %d \n", newfd);
						}
					}
					else{
						//here we get data from sub_processes or from RTnet
						printf("Space: %d \n", free_space);
						//add new packet to the end of all data
						nbytes = recv(i, input_buffer+inputWrittenIndex, free_space, 0);

						if ( nbytes <= 0 ){
							close(i);
							FD_CLR(i, &master);
						}
						else{
							if (free_space >= ( nbytes + sizeof( (uint8_t) i )) ){
								AddClientIDToPacket(input_buffer+inputWrittenIndex, (uint8_t) i,  &inputWrittenIndex, nbytes);
								inputWrittenIndex += nbytes;
								licznik += 1 ;
							}
							//we need to erase memory which is too small to send data (the last packet)
							else{
								memset(input_buffer+inputWrittenIndex, 0, free_space);
								//better never be here!
							}
						}
					}
				}
			}
    	}
    	if(rv < 0){
    	    perror("select"); // error occurred in select()
    	}
    	if(rv == 0){
            //here make other stuff
    		//reorganize data,
    		//pack everything together and wait for time slot to send everything to RTnet
    		//
    		// unpack data from RTnet
    		// resend data to subprocesses
    		if(licznik > 10){
    			printf("Dane: %s \n", input_buffer);
    			memset(input_buffer, 0, sizeof(input_buffer));
    			inputWrittenIndex = 0;
    			licznik = 0 ;
    		}
    	}
    }
    return 0;
}
 */

/*
 * MOPS_RTnet_Con.c
 *
 *  Created on: Jan 30, 2016
 *      Author: rudy
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include "MOPS_RTnet_Con.h"
#include "MOPS.h"


static struct sockaddr_in remote;

#if TARGET_DEVICE == Linux
int connectToRTnet(){
	int bcast_sock;
	int enable = 1;
    if ((bcast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket");
    }
    setsockopt(bcast_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    setsockopt(bcast_sock, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));
    memset(&remote, 0, sizeof(remote));

    remote.sin_family = AF_INET;
    remote.sin_port = htons(PORT);
    remote.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	if (bind(bcast_sock, &remote, sizeof(remote))==-1)
		perror("bind");

    return bcast_sock;
}

void sendToRTnet(int socket, uint8_t *buf, int buflen){
	socklen_t len = sizeof(remote);
	int write = 0;
    remote.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if((write = sendto(socket, buf, buflen, 0, &remote, len)) < 0)
        perror("sendto");
}


int receiveFromRTnet(int socket, uint8_t *buf, int buflen){
	int written = 0;
	socklen_t len = sizeof(remote);
	written = recvfrom(socket, buf, buflen, 0, &remote, &len);
	return written;
}


pthread_t startNewThread(void *(*start_routine) (void *), void *arg){
	int err;
	pthread_t thread_id;
    err = pthread_create(&thread_id, NULL, start_routine, arg);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));
    return thread_id;
}

uint8_t mutex_init(pthread_mutex_t *lock){
    if (pthread_mutex_init(lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    return 0;
}
void lock_mutex(pthread_mutex_t *lock){
	pthread_mutex_lock(lock);
}
void unlock_mutex(pthread_mutex_t *lock){
	pthread_mutex_unlock(lock);
}
#endif //TARGET == LINUX



#if TARGET_DEVICE == RTnode
void startNewThread(void *(*start_routine) (void *), void *arg){

}

int connectToRTnet(){
	int bcast_sock = 1;
    return bcast_sock;
}

void sendToRTnet(int socket, uint8_t *buf, int buflen){
	return;
}
int receiveFromRTnet(int socket, uint8_t *buf, int buflen){
	return 0;
}

uint8_t mutex_init(SemaphoreHandle_t *lock){
	*lock = xSemaphoreCreateMutex();
	if( *lock == NULL )
		return 1;
    return 0;
}

void lock_mutex(SemaphoreHandle_t *lock){
	while( xSemaphoreTake( *lock, ( TickType_t ) 100.0f/TICK_PERIOD_MS ) != pdTRUE )
	{};
}
void unlock_mutex(SemaphoreHandle_t *lock){
	xSemaphoreGive( *lock );
}
#endif //TARGET == RTNODE



//***************** MOPS - MOPS communication protocol ********************
uint16_t buildTopicRequestMessage(uint8_t *Buffer, int BufferLen){
	MOPSHeader MHeader;
	uint8_t index = 0, tempLen = 0;

	MHeader.MOPSMessageType = TOPIC_REQUEST;
	MHeader.RemainingLengthLSB = 0;
	MHeader.RemainingLengthMSB = 0;
	memcpy(Buffer, &MHeader, sizeof(MHeader));
	index += sizeof(MHeader);

	return index;
}


uint16_t buildNewTopicMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topics, uint16_t *IDs, int TopicNo){
	MOPSHeader MHeader;
	uint8_t MSB_temp, LSB_temp;
	int index = 0, tempLen = 0, i;
	index = sizeof(MHeader);

	//**** Payload part *****//
	for (i=0; i<TopicNo; i++){
		u16ToMSBandLSB(IDs[i], &MSB_temp, &LSB_temp);
		Buffer[index] = MSB_temp;
		Buffer[index+1] = LSB_temp;
		tempLen = strlen(Topics[i]);
		u16ToMSBandLSB(tempLen, &MSB_temp, &LSB_temp);
		Buffer[index+2] = MSB_temp;
		Buffer[index+3] = LSB_temp;
		index += 4;
		memcpy(Buffer + index, Topics[i], tempLen);
		index += tempLen;
	}
	//**** Payload part *****//

	u16ToMSBandLSB(index-sizeof(MHeader), &MSB_temp, &LSB_temp);
	MHeader.MOPSMessageType = NEW_TOPICS;
	MHeader.RemainingLengthMSB = MSB_temp;
	MHeader.RemainingLengthLSB = LSB_temp;
	memcpy(Buffer, &MHeader, sizeof(MHeader));

	return (uint16_t)index;
}


uint16_t buildEmptyMessage(uint8_t *Buffer, int BufferLen){
	MOPSHeader MHeader;
	uint8_t index = 0, tempLen = 0;

	MHeader.MOPSMessageType = NOTHING;
	MHeader.RemainingLengthLSB = 0;
	MHeader.RemainingLengthMSB = 0;
	memcpy(Buffer, &MHeader, sizeof(MHeader));
	index += sizeof(MHeader);

	return index;
}
//*********************************************************************

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
#include <sys/un.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include "MOPS_RTnet_Con.h"
#include "MOPS.h"
#include <rtnet.h>
#include <rtmac.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>

#define IPADDR     "10.255.255.255"
#define IPADDR_LO  "127.0.0.1"


static struct sockaddr_in rec_addr;
static struct sockaddr_in sd_addr_b;
static struct sockaddr_in sd_addr_l;
int get_sock;
int bcast_sock;

#if TARGET_DEVICE == Linux
void connectToRTnet(){
	int enable = 1;
    if ((bcast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        perror("socket");
    if ((get_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            perror("socket");

    memset(&rec_addr, 0, sizeof(rec_addr));
    memset(&sd_addr_b, 0, sizeof(sd_addr_b));
    memset(&sd_addr_l, 0, sizeof(sd_addr_l));

    rec_addr.sin_family = AF_INET;
    rec_addr.sin_port = htons(PORT);
    rec_addr.sin_addr.s_addr =  htonl(INADDR_ANY);//inet_addr("10.0.0.0");

    sd_addr_b.sin_family = AF_INET;
    sd_addr_b.sin_port = htons(PORT);
    sd_addr_b.sin_addr.s_addr =  inet_addr(IPADDR);

    sd_addr_l.sin_family = AF_INET;
    sd_addr_l.sin_port = htons(PORT);
    sd_addr_l.sin_addr.s_addr =  inet_addr(IPADDR_LO);

	if (bind(get_sock, &rec_addr, sizeof(rec_addr))==-1)
		perror("bind");
}

void sendToRTnet(uint8_t *buf, int buflen){
	int write = 0;
	socklen_t len = sizeof(sd_addr_b);
    if((write = sendto(bcast_sock, buf, buflen, 0, &sd_addr_b, len)) < 0)
        perror("sendto");
    if((write = sendto(bcast_sock, buf, buflen, 0, &sd_addr_l, len)) < 0)
        perror("sendto");
}

int receiveFromRTnet(uint8_t *buf, int buflen){
	int written = 0;
	socklen_t len = sizeof(rec_addr);
	written = recvfrom(get_sock, buf, buflen, 0, &rec_addr, &len);
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

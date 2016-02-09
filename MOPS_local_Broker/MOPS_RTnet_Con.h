/*
 * MOPS_RTnet_Con.h
 *
 *  Created on: Jan 30, 2016
 *      Author: rudy
 */

#ifndef MOPS_RTNET_CON_H_
#define MOPS_RTNET_CON_H_

#include <pthread.h>

#define Linux  1
#define RTnode 2

//***************Settings********************
#define TARGET_DEVICE Linux  //or RTnode
#define PORT 1883
#define UDP_MAX_SIZE 512

#define MAX_TOPIC_LENGTH             30      //max is 2^16-1
#define MAX_NUMBER_OF_TOPIC          8       //max is 2^16-1
#define MAX_NUMBER_OF_SUBSCRIPTIONS  100     //max is 2^16-1
//***************Settings********************


//**** MOPS - MOPS communication protocol ****
enum MOPS_MESSAGES{
	TOPIC_REQUEST = 1,
	NEW_TOPICS,
	NOTHING,
};

typedef struct MOPSHeader{
	uint8_t MOPSMessageType;
	uint8_t RemainingLengthMSB;
	uint8_t RemainingLengthLSB;
}MOPSHeader;

void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB);
uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB);
uint16_t buildTopicRequestMessage(uint8_t *Buffer, int BufferLen);
uint16_t buildNewTopicMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topics, uint16_t *IDs, int TopicNo);
uint16_t buildEmptyMessage(uint8_t *Buffer, int BufferLen);
//********************************************



int connectToRTnet();
int receiveFromRTnet(int socket, uint8_t *buf, int buflen);
void sendToRTnet(int socket, uint8_t *buf, int buflen);

#if TARGET_DEVICE == Linux
pthread_t startNewThread(void *(*start_routine) (void *), void *arg);
uint8_t mutex_init(pthread_mutex_t *lock);
void lock_mutex(pthread_mutex_t *lock);
void unlock_mutex(pthread_mutex_t *lock);
#endif //TARGET_DEVICE == Linux


#if TARGET_DEVICE == RTnode
void startNewThread(void *(*start_routine) (void *), void *arg);
uint8_t mutex_init(SemaphoreHandle_t *lock);
void lock_mutex(SemaphoreHandle_t *lock);
void unlock_mutex(SemaphoreHandle_t *lock);
#endif //TARGET_DEVICE == RTnode

#endif /* MOPS_RTNET_CON_H_ */

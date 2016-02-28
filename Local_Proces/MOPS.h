/*
 * MOPS.h
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#ifndef MOPS_H_
#define MOPS_H_
#include <stdint.h>
#include <mqueue.h>

#define Linux  1
#define RTnode 2
#define TARGET_DEVICE Linux   //or RTNODE

#define MAX_QUEUE_SIZE 100
#define MAX_QUEUE_MESSAGE 10


int connectMOPS();
int sendToMOPS(uint8_t *buffer, uint16_t buffLen);
int recvFromMOPS(uint8_t *buffer, uint16_t buffLen);

void publishMOPS(int fd, uint8_t *Topic, uint8_t *Message);
void subscribeMOPS(uint8_t **TopicList, uint8_t *QosList, uint8_t NoOfTopics);
int readMOPS(uint8_t *buf, uint8_t length);
int InterpretFrame(uint8_t *messageBuf, uint8_t *frameBuf, uint8_t frameLen);

void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB);
uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB);

#if TARGET_DEVICE == Linux
#define QUEUE_NAME "/MOPS_path"

typedef struct MOPS_Queue{
	mqd_t  	ProcesToMOPS_fd;
	mqd_t  	MOPSToProces_fd;
}MOPS_Queue;

#endif




#endif /* MOPS_H_ */

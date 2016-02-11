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



int connectMOPS();
int sendToMOPS(uint8_t *buffer, uint16_t buffLen);
int recvFromMOPS(uint8_t *buffer, uint16_t buffLen);

void publishMOPS(int fd, uint8_t *Topic, uint8_t *Message);
void subscribeMOPS(uint8_t **TopicList, uint8_t **QosList);
int readMOPS(int fd, uint8_t *buf, uint8_t length);

#if TARGET_DEVICE == Linux
#define QUEUE_NAME "/MOPS_path"
#define MAX_QUEUE_SIZE 100

typedef struct MOPS_Queue{
	mqd_t  	ProcesToMOPS_fd;
	mqd_t  	MOPSToProces_fd;
}MOPS_Queue;

#endif




#endif /* MOPS_H_ */

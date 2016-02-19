/*
 * MOPS.c
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <mqueue.h>
#include "MOPS.h"
#include "MQTT.h"


//static int file_desc;
//static struct sockaddr_un remote;
static MOPS_Queue mops_queue;

#if TARGET_DEVICE == Linux
int connectMOPS(){
	uint8_t temp;
    mqd_t mq;
    struct mq_attr attr;
    char buffer[10] = {'/',0,0,0,0,0,0,0,0,0};
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_QUEUE_SIZE;
    attr.mq_curmsgs = 0;
    sprintf(buffer+1, "%d", getpid());

    mq = mq_open(QUEUE_NAME, O_WRONLY);
    if( !((mqd_t)-1 != mq) )
    	perror("MQueue Open");

	temp = strlen(buffer);
	buffer[temp] = 'a';
    mops_queue.MOPSToProces_fd = mq_open(buffer, O_CREAT | O_RDONLY, 0644, &attr);
    if( !((mqd_t)-1 != mops_queue.MOPSToProces_fd) )
    	perror("MQueue Open MOPSToProces");
	buffer[temp] = 'b';
    mops_queue.ProcesToMOPS_fd = mq_open(buffer, O_CREAT | O_WRONLY, 0644, &attr);
    if( !((mqd_t)-1 != mops_queue.ProcesToMOPS_fd) )
    	perror("MQueue Open ProcesToMOPS");

    buffer[temp] = 0;
    if( !(0 <= mq_send(mq, buffer, 10, 0)) )
    	perror("Send MQueue");
    if( !((mqd_t)-1 != mq_close(mq)) )
    	perror("Close MQueue");

    return 0;
}
/*int file_desc;
if ((file_desc = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    perror("socket");

printf("Trying to connect...\n");
remote.sun_family = AF_UNIX;
strcpy(remote.sun_path, SOCK_PATH);
if (connect(file_desc, (struct sockaddr *)&remote, strlen(remote.sun_path) + sizeof(remote.sun_family)) == -1)
    perror("connect");

printf("Connected.\n");
return file_desc; */



int sendToMOPS(uint8_t *buffer, uint16_t buffLen){
	return mq_send(mops_queue.ProcesToMOPS_fd, buffer, buffLen, 0);
}

int recvFromMOPS(uint8_t *buffer, uint16_t buffLen){
	return mq_receive(mops_queue.MOPSToProces_fd, buffer, buffLen, NULL);
}
#endif //TARGET_DEVICE == Linux

#if TARGET_DEVICE == RTnode
int connectMOPS(){}
int sendToMOPS(int fd, uint8_t *buffer, uint16_t buffLen){}
int recvFromMOPS(int fd, uint8_t *buffer, uint16_t buffLen){}
#endif //TARGET_DEVICE == RTnode

void publishMOPS(int fd, uint8_t *Topic, uint8_t *Message){
	uint8_t buffer[MAX_QUEUE_SIZE];
	memset(buffer, 0, MAX_QUEUE_SIZE);
	uint16_t packetID, written;
	written = BuildClientPublishMessage(buffer, sizeof(buffer), Topic, Message, 0, 0, &packetID);
    if (sendToMOPS(buffer, written) == -1) {
        perror("send");
    }
}

void subscribeMOPS(uint8_t **TopicList, uint8_t *QosList, uint8_t NoOfTopics){
	uint8_t buffer[MAX_QUEUE_SIZE];
	memset(buffer, 0, MAX_QUEUE_SIZE);
	uint16_t packetID, written;
	written = BuildSubscribeMessage(buffer, sizeof(buffer), TopicList, QosList, NoOfTopics, &packetID);

    if (sendToMOPS(buffer, written) == -1) {
        perror("send");
    }
}

int readMOPS(uint8_t *buf, uint8_t length){
	uint8_t temp[MAX_QUEUE_SIZE];
    int t;
	memset(temp,0,MAX_QUEUE_SIZE);
	memset(buf,0,length);

	if ((t=recvFromMOPS(temp, MAX_QUEUE_SIZE)) > 0) {
		return InterpretFrame(buf, temp, t);
    } else {
        if (t < 0) perror("recv");
        else printf("Server closed connection\n");
    }
    return t;
}

int InterpretFrame(uint8_t *messageBuf, uint8_t *frameBuf, uint8_t frameLen){
	FixedHeader FHeader;
	uint8_t Qos, topicLen, messsageLen;
	uint16_t headLen = 0, index = 3;

	headLen = sizeof(FHeader);
	memcpy(&FHeader, frameBuf, headLen);
	Qos = (FHeader.Flags & 6) >> 1;

	topicLen = MSBandLSBTou16(frameBuf[index], frameBuf[index+1]);
	index += (2+topicLen);
	if(Qos > 0)
		index += 2;
	messsageLen = MSBandLSBTou16(frameBuf[index], frameBuf[index+1]);
	index += 2;
	if( (index+messsageLen) == frameLen){
		memcpy(messageBuf, frameBuf+index, messsageLen);
		return messsageLen;
	}
	return 0;
}

void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB){
	uint16_t temp;
	*LSB = (uint8_t) u16bit;
	temp = u16bit>>8;
	*MSB = (uint8_t) temp;
}

uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB){
	uint16_t temp;
	temp = MSB;
	temp = temp<<8;
	temp += LSB;
	return temp;
}

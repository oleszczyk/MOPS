/*
 * MOPS.c
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#include <stdio.h>
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
    if( !(0 <= mq_send(mq, buffer, 10, 0)) )
    	perror("Send MQueue");
    if( !((mqd_t)-1 != mq_close(mq)) )
    	perror("Close MQueue");

    mops_queue.MOPSToProces_fd = mq_open(buffer, O_CREAT | O_RDONLY, 0644, &attr);
    if( !((mqd_t)-1 != mops_queue.MOPSToProces_fd) )
    	perror("MQueue Open MOPSToProces");
    mops_queue.ProcesToMOPS_fd = mq_open(buffer, O_WRONLY);
    if( !((mqd_t)-1 != mops_queue.ProcesToMOPS_fd) )
    	perror("MQueue Open ProcesToMOPS");

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
	static uint8_t buffer[100];
	memset(buffer, 0, sizeof(buffer));
	uint16_t packetID, written;
	written = BuildClientPublishMessage(buffer, sizeof(buffer), Topic, Message, 0, 0, &packetID);
    if (sendToMOPS(buffer, written) == -1) {
        perror("send");
    }
}

void subscribeMOPS(uint8_t **TopicList, uint8_t **QosList){
	printf("Subskrybent! \n");
}

int readMOPS(int fd, uint8_t *buf, uint8_t length){
    int t;
	if ((t=recvFromMOPS(buf, length)) > 0) {
		buf[t] = '\0';
    } else {
        if (t < 0) perror("recv");
        else printf("Server closed connection\n");
    }
    return t;
}

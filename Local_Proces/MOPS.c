/*
 * MOPS.c
 *
 *  Created on: Jan 20, 2016
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
#include "MOPS.h"

//static int file_desc;
static int state = 0;
static struct sockaddr_un remote;

int connectMOPS(){
	int file_desc;
    if ((file_desc = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
    }

    printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);

    if (connect(file_desc, (struct sockaddr *)&remote, strlen(remote.sun_path) + sizeof(remote.sun_family)) == -1) {
        perror("connect");
        exit(1);
    }
    printf("Connected.\n");
    return file_desc;
}



void publishMOPS(int fd, uint8_t *Topic, uint8_t *Message, uint8_t length){
    if (send(fd, Topic, length, 0) == -1) {
        perror("send");
        exit(1);
    }
}

void subscribeMOPS(uint8_t **TopicList, uint8_t **QosList){
	printf("Subskrybent! \n");
}

int readMOPS(int fd, uint8_t *buf, uint8_t length){
    int t;

	if ((t=recv(fd, buf, length, 0)) > 0) {
		buf[t] = '\0';
    } else {
        if (t < 0) perror("recv");
        else printf("Server closed connection\n");
    }
    return t;
}


int BindSocket();


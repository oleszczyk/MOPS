/*
 * MOPS.h
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#ifndef MOPS_H_
#define MOPS_H_
#include <stdint.h>


#define SOCK_PATH "./../MOPS_path"

#define TARGET LINUX   //or RTNODE



int connectMOPS();
void publishMOPS(int fd, uint8_t *Topic, uint8_t *Message, uint8_t length);
void subscribeMOPS(uint8_t **TopicList, uint8_t **QosList);
int readMOPS(int fd, uint8_t *buf, uint8_t length);





#endif /* MOPS_H_ */

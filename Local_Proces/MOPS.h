/*
 * MOPS.h
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#ifndef MOPS_H_
#define MOPS_H_
#include <stdint.h>

int connectMOPS();
void publishMOPS(uint8_t *Topic, uint8_t *Message, uint8_t length);
void subscribeMOPS(uint8_t **TopicList, uint8_t **QosList);
int readMOPS(int fd, uint8_t *buf, uint8_t length);


#endif /* MOPS_H_ */

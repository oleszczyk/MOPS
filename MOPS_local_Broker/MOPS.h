/*
 * MOPS.h
 *
 *  Created on: Jan 31, 2016
 *      Author: rudy
 */

#ifndef MOPS_H_
#define MOPS_H_

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include "MOPS_RTnet_Con.h"

enum MOPS_STATE{
	SEND_NOTHING = 1,
	SEND_REQUEST,
	SEND_TOPIC_LIST,
	SEND_NEW_TOPIC,
};


void AddClientIDToPacket(uint8_t *buf, uint8_t ClientID, int *WrittenBytes, int nbytes);
void InitTopicList(TopicID list[]);
uint16_t SendTopicList(uint8_t *Buffer, int BufferLen, TopicID list[]);
uint16_t SendLocalTopics(uint8_t *Buffer, int BufferLen, TopicID list[]);

uint8_t AddTopicToList(TopicID list[], uint8_t *topic, uint16_t topicLen, uint16_t id);
void threadAction(int RTsocket);
void AnalizeIncomingUDP(uint8_t *Buffer, uint8_t BufferLen);
void UpdateTopicList(uint8_t *Buffer, uint8_t BufferLen);
uint8_t ApplyIDtoNewTopics();

void AddTopicCandidate(uint8_t *topic, uint16_t topicLen);

/*
 * return:
 *  ID (uint16_t value) if topic exist already in TopicList and is available
 *  0					if topic is candidate in TopicList
 *  -1					if topic is not available or candidate
 */
int GetIDfromTopicName(uint8_t *topic, uint16_t topicLen);

/*
 * POST: variable 'topic' is set as Topic with id 'id',
 * if there is not a topic in TopicList with that id
 * variable 'topic' is set to \0.
 */
void GetTopicNameFromID(uint16_t id, uint8_t *topic, uint16_t topicLen);

#endif /* MOPS_H_ */

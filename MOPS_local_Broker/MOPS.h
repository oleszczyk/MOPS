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
#include <mqueue.h>
#include "MOPS_RTnet_Con.h"
#include "MQTT.h"


typedef struct TopicID{
	uint8_t Topic[MAX_TOPIC_LENGTH+1];
	uint16_t ID;
	uint8_t LocalTopic;	//flag - if 1 then topic has to be send to RTnet
}TopicID;

typedef struct SubscriberList{
	uint8_t Topic[MAX_TOPIC_LENGTH+1];
	int ClientID;
}SubscriberList;

enum MOPS_STATE{
	SEND_NOTHING = 1,
	SEND_REQUEST,
	SEND_TOPIC_LIST,
};


#if TARGET_DEVICE == Linux
#define QUEUE_NAME "/MOPS_path"
#define MAX_PROCES_CONNECTION 10
#define MAX_QUEUE_SIZE 100

typedef struct MOPS_Queue{
	mqd_t  	ProcesToMOPS_fd;
	mqd_t  	MOPSToProces_fd;
}MOPS_Queue;
#endif //TARGET_DEVICE == Linux

#if TARGET_DEVICE == RTnode
#define MAX_PROCES_CONNECTION 10 //number of local processes connected to MOPS broker
typedef struct MOPS_Queue{

}MOPS_Queue;
#endif //TARGET_DEVICE == RTnode


void threadSendToRTnet(int RTsocket);
void threadRecvFromRTnet(int RTsocket);
void threadRecvFromProcess(int socket);


void AddClientIDToPacket(uint8_t *buf, uint8_t ClientID, int *WrittenBytes, int nbytes);
void InitTopicList(TopicID list[]);

uint16_t SendEmptyMessage(uint8_t *Buffer, int BufferLen);
uint16_t SendTopicRequestMessage(uint8_t *Buffer, int BufferLen);
uint16_t SendTopicList(uint8_t *Buffer, int BufferLen, TopicID list[]);
uint16_t SendLocalTopics(uint8_t *Buffer, int BufferLen, TopicID list[]);

uint8_t AddTopicToList(TopicID list[], uint8_t *topic, uint16_t topicLen, uint16_t id);
void AnalyzeIncomingUDP(uint8_t *Buffer, int written_bytes);
void UpdateTopicList(uint8_t *Buffer, int BufferLen);
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
uint16_t GetTopicNameFromID(uint16_t id, uint8_t *topic);

void InitProcesConnection();
int AddToMOPSQueue(int MOPS_Proces_fd, int Proces_MOPS_fd);
void MOPS_QueueInit(MOPS_Queue *queue);
int AddToSubscribersList(uint8_t *topic, uint16_t topicLen, int ClientID);
void PrepareFrameToSendToProcess(uint8_t *Buffer, int written_bytes);
int ReceiveFromProcess(int file_de);
int SendToProcess(uint8_t *buffer, uint16_t buffLen, int file_de);
int ServeSendingToProcesses();
int FindClientIDbyFileDesc(int file_de);
int FindClientIDbyTopic(uint8_t *topic, uint16_t topicLen);


void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB);
uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB);

void AnalyzeProcessMessage(uint8_t *buffer, int bytes_wrote, int ClientID);
void ServePublishMessage(uint8_t *buffer, int FrameLen);
void ServeSubscribeMessage(uint8_t *buffer, int FrameLen, int ClientID);
void AddPacketToWaitingTab(uint8_t *buffer, int FrameLen);
void AddPacketToFinalTab(uint8_t *buffer, int FrameLen, uint16_t topicID);
void MoveWaitingToFinal();

#endif /* MOPS_H_ */

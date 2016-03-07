/*
 * MOPS.h
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#ifndef MOPS_H_
#define MOPS_H_
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <mqueue.h>
#include "MQTT.h"

#define Linux  1
#define RTnode 2

//***************** General Settings *********************
#define TARGET_DEVICE Linux   //or RTNODE
#define MAX_PROCES_CONNECTION 10
#define MAX_QUEUE_SIZE 100
#define MAX_QUEUE_MESSAGE 10
//***************** General Settings *********************

//***************MOPS - RTnet Settings********************
#define PORT 1883
#define UDP_MAX_SIZE 512

#define IPADDR     "10.255.255.255"
#define IPADDR_LO  "127.0.0.1"
#define MAX_TOPIC_LENGTH             10      //max is 2^16-1
#define MAX_MESSAGE_LENGTH			 100	 //max is 2^16-1
#define MAX_NUMBER_OF_TOPIC          8       //max is 2^16-1
#define MAX_NUMBER_OF_SUBSCRIPTIONS  100     //max is 2^16-1
//***************MOPS - RTnet Settings********************

#if TARGET_DEVICE == Linux
#define QUEUE_NAME "/MOPS_path"

typedef struct MOPS_Queue {
	mqd_t ProcesToMOPS_fd;
	mqd_t MOPSToProces_fd;
} MOPS_Queue;
#endif //TARGET_DEVICE == Linux
#if TARGET_DEVICE == RTnode
typedef struct MOPS_Queue {

}MOPS_Queue;
#endif //TARGET_DEVICE == RTnode
typedef struct TopicID {
	uint8_t Topic[MAX_TOPIC_LENGTH + 1];
	uint16_t ID;
	uint8_t LocalTopic; //flag - if 1 then topic has to be send to RTnet
} TopicID;

typedef struct SubscriberList {
	uint8_t Topic[MAX_TOPIC_LENGTH + 1];
	int ClientID;
} SubscriberList;

enum MOPS_STATE {
	SEND_NOTHING = 1, SEND_REQUEST, SEND_TOPIC_LIST,
};

// ***************   Funtions for local processes   ***************//
int connectToMOPS();
int sendToMOPS(char *buffer, uint16_t buffLen);
int recvFromMOPS(char *buffer, uint16_t buffLen);

void publishMOPS(char *Topic, char *Message);
void subscribeMOPS(char **TopicList, uint8_t *QosList, uint8_t NoOfTopics);
int readMOPS(char *buf, uint8_t length);
int InterpretFrame(char *messageBuf, char *frameBuf, uint8_t frameLen);
// ***************   Funtions for local processes   ***************//

// ***************   Funtions for local MOPS broker   ***************//
void threadSendToRTnet();
void threadRecvFromRTnet();

void AddClientIDToPacket(uint8_t *buf, uint8_t ClientID, int *WrittenBytes,
		int nbytes);
void InitTopicList(TopicID list[]);
void SubListInit(SubscriberList *sublist);

uint16_t SendEmptyMessage();
uint16_t SendTopicRequestMessage();
uint16_t SendTopicList(TopicID list[]);
uint16_t SendLocalTopics(TopicID list[]);

uint8_t AddTopicToList(TopicID list[], uint8_t *topic, uint16_t topicLen,
		uint16_t id);
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
int ServeNewProcessConnection(fd_set *set, int listener_fd);
void CloseProcessConnection(int file_de);
int AddToMOPSQueue(int MOPS_Proces_fd, int Proces_MOPS_fd);
void MOPS_QueueInit(MOPS_Queue *queue);
int AddToSubscribersList(uint8_t *topic, uint16_t topicLen, int ClientID);
void PrepareFrameToSendToProcess(uint8_t *Buffer, int written_bytes);
int ReceiveFromProcess(int file_de);
int SendToProcess(uint8_t *buffer, uint16_t buffLen, int file_de);
int ServeSendingToProcesses();
int FindClientIDbyFileDesc(int file_de);
void FindClientsIDbyTopic(int *clientsID, uint8_t *topic, uint16_t topicLen);
void AnalyzeProcessMessage(uint8_t *buffer, int bytes_wrote, int ClientID);
void ServePublishMessage(uint8_t *buffer, int FrameLen);
void ServeSubscribeMessage(uint8_t *buffer, int FrameLen, int ClientID);
void AddPacketToWaitingTab(uint8_t *buffer, int FrameLen);
void AddPacketToFinalTab(uint8_t *buffer, int FrameLen, uint16_t topicID);
void MoveWaitingToFinal();
// ***************   Funtions for local MOPS broker   ***************//

void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB);
uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB);

#endif /* MOPS_H_ */

/**
 *	@file	MOPS.h
 *	@date	Mar 10, 2016
 *	@author	Michal Oleszczyk
 *	@brief	File defines, structs, enums and function headers.
 *
 *	Headers set of functions for broker-process communication
 *	and broker logic in general. Here are also stored all defines
 *	responsible for main configuration of MOPS parameters.
 */

#ifndef MOPS_H_
#define MOPS_H_

/** Linux target value. */
#define Linux  1
/** RTnode target value. */
#define RTnode 2

//***************** General Settings *********************
/** Target on which we want to build MOPS library (Linux/RTnode). */
#define TARGET_DEVICE Linux
/** Maximal number of connected local processes to broker. */
#define MAX_PROCES_CONNECTION 100
/** Maximal length of message broker<->process. */
#define MAX_QUEUE_MESSAGE_SIZE 100
/** Maximal amount of messages stored in queues broker<->process. */
#define MAX_QUEUE_MESSAGE_NUMBER 10
//***************** General Settings *********************

//***************MOPS - RTnet Settings********************
/** MOPS protocol port. */
#define MOPS_PORT 1525
/** Size of send/receive buffers. */
#define UDP_MAX_SIZE 512

/** Broadcast address. */
#define IPADDR     "10.255.255.255"
/** Maximal length of MOPS topic name (max is 2^16-1).*/
#define MAX_TOPIC_LENGTH             30
/** Maximal length of MOPS message (max is 2^16-1).*/
#define MAX_MESSAGE_LENGTH			 100
/** Maximal number of different topic names (max is 2^16-1).*/
#define MAX_NUMBER_OF_TOPIC          100
/** Maximal number of different subscriptions (max is 2^16-1).*/
#define MAX_NUMBER_OF_SUBSCRIPTIONS  100
//***************MOPS - RTnet Settings********************

#if TARGET_DEVICE == Linux
#include <mqueue.h>

/** Name of general queue (processes->broker). */
#define QUEUE_NAME "/MOPS_path"

/**
 * @struct MOPS_Queue
 * @brief Structure for connecting two file descriptors
 * responsible for broker<->process communication.
 *
 * Each new local process which wants to connect to MOPS
 * broker, create to queues with format: \{proces_id}a,
 * \{proces_id}b. First one (a) is for broker->process, second
 * one (b) is process->broker. This structure is used to build
 * 'communication list'.
 * */
typedef struct MOPS_Queue {
	/** File descriptor for transmission process->broker*/
	mqd_t ProcesToMOPS_fd;
	/** File descriptor for transmission broker->process*/
	mqd_t MOPSToProces_fd;
} MOPS_Queue;
#endif //TARGET_DEVICE == Linux
#if TARGET_DEVICE == RTnode
#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "rtnet.h"
#include "rtnet_inet.h"

QueueHandle_t GlobalProcesMopsQueue;

typedef struct MOPS_Queue {
	QueueHandle_t ProcesToMOPS_fd;
	QueueHandle_t MOPSToProces_fd;
}MOPS_Queue;
#endif //TARGET_DEVICE == RTnode

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include "MQTT.h"

/**
 * @struct TopicID
 * @brief Structure links topic name and its ID.
 *
 * This type is used to create 'topic list'. This struct
 * contain information about already known topics (its ID),
 * and about 'candidates' (ID=0). Moreover if field
 * 'LocalTopic' is set, topic has to be announced to
 * all RTnet participants.
 */
typedef struct TopicID {
	/** Topic name - at most MAX_TOPIC_LENGTH long string. */
	uint8_t Topic[MAX_TOPIC_LENGTH + 1];
	/** ID of this topic. */
	uint16_t ID;
	/** If this field is equal 1, this topic has to be announced in RTnet. */
	uint8_t LocalTopic;
} TopicID;

/**
 * @struct SubscriberList
 * @brief Structure creating subscriptions.
 *
 * This type is used in 'subscription list'.
 * Struct contain topic name and clients ID of
 * processes which subscribe this particular topic.
 */
typedef struct SubscriberList {
	/** Topic name - at most MAX_TOPIC_LENGTH long string. */
	uint8_t Topic[MAX_TOPIC_LENGTH + 1];
	/** Client ID of process which subscribe this topic. */
	int ClientID;
} SubscriberList;

/**
 * @enum MOPS_STATE
 * @brief State of MOPS broker.
 *
 * Describes how MOPS broker should behavior, which MOPS protocol header
 * should build and send to RTnet.
 * */
enum MOPS_STATE {
	SEND_NOTHING = 1,/**< Usual state - process as usual, receive and send messages. */
	SEND_REQUEST,    /**< Init state - responsible for request for all known topics list. */
	SEND_TOPIC_LIST, /**< Topics respond - set when some RTnet participant requested for full topic list.*/
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
int StartMOPSBroker();
int StartMOPSBrokerNonBlocking();
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
int GetIDfromTopicName(uint8_t *topic, uint16_t topicLen);
uint16_t GetTopicNameFromID(uint16_t id, uint8_t *topic);
void InitProcesConnection();
#if TARGET_DEVICE == Linux
int ServeNewProcessConnection(fd_set *set, int listener_fd);
#endif
#if TARGET_DEVICE == RTnode
QueueHandle_t ServeNewProcessConnection();
#endif
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

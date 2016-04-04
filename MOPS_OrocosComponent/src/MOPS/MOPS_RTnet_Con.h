/**
 *	@brief	Header file containing for communication between
 *	MOPS brokers in RTnet.
 *
 *	Headers of function and structures definitions for broker-broker
 *	communication. Communication is based on UDP transfer. Every
 *	broker is sending its UDP packet to broadcast address and to
 *	yourself on port 1883.
 *
 *	@file	MOPS_RTnet_Con.h
 *	@date	Jan 30, 2016
 *	@author	Michal Oleszczyk
 */

#ifndef MOPS_RTNET_CON_H_
#define MOPS_RTNET_CON_H_

#include "MOPS.h"
#include <pthread.h>


//**** MOPS - MOPS communication protocol ****
/**
 * @enum MOPS_MESSAGES
 * @brief MOPS protocol header types.
 *
 * Describes type of MOPS protocol header.
 */
enum MOPS_MESSAGES{
	TOPIC_REQUEST = 1,/**< Sent after broker start. Request for full topic list from other brokers. */
	NEW_TOPICS,       /**< Means that in MOPS message payload are announced new topics. */
	NOTHING,          /**< Header does not contain any useful information. */
};

/**
 * @struct MOPSHeader
 * @brief MOPS protocol header base.
 *
 * This structure contain main fields which all MOPS protocol headers
 * have to have. In case of header "New Topic", after that base will be
 * header payload containing topic ID, topic length and topic itself.
 */
typedef struct MOPSHeader{
	uint8_t MOPSMessageType;   /**< type: 1-TOPIC_REQUEST, 2-NEW_TOPICS, 3-NOTHING*/
	uint8_t RemainingLengthMSB;/**< remaining length of MOPS header, most significant byte of 16bit value.*/
	uint8_t RemainingLengthLSB;/**< remaining length of MOPS header, least significant byte of 16bit value.*/
}MOPSHeader;

uint16_t buildTopicRequestMessage(uint8_t *Buffer, int BufferLen);
uint16_t buildNewTopicMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topics, uint16_t *IDs, int TopicNo);
uint16_t buildEmptyMessage(uint8_t *Buffer, int BufferLen);
//********************************************



void connectToRTnet();
int receiveFromRTnet(uint8_t *buf, int buflen);
void sendToRTnet(uint8_t *buf, int buflen);

#if TARGET_DEVICE == Linux
pthread_t startNewThread(void *(*start_routine) (void *), void *arg);
uint8_t mutex_init(pthread_mutex_t *lock);
void lock_mutex(pthread_mutex_t *lock);
void unlock_mutex(pthread_mutex_t *lock);
#endif //TARGET_DEVICE == Linux


#if TARGET_DEVICE == RTnode
void startNewThread(void *(*start_routine) (void *), void *arg);
uint8_t mutex_init(SemaphoreHandle_t *lock);
void lock_mutex(SemaphoreHandle_t *lock);
void unlock_mutex(SemaphoreHandle_t *lock);
#endif //TARGET_DEVICE == RTnode

#endif /* MOPS_RTNET_CON_H_ */

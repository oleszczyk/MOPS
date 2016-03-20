/**
 *	@brief	File containing function responsible for
 *			communication between MOPS brokers in RTnet.
 *
 *	Implementation for set of function for broker-broker communication.
 *	Communication is based on UDP transfer. Every broker is sending its
 *	UDP packet to broadcast address and to yourself on port 1883.
 *
 *	@file	MOPS_RTnet_Con.c
 *	@date	Jan 30, 2016
 *	@author	Michal Oleszczyk
 */
#include "MOPS.h"
#include "MOPS_RTnet_Con.h"

#if TARGET_DEVICE == Linux
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>
#endif //TARGET_DEVICE == Linux

#if TARGET_DEVICE == RTnode
#include "task.h"
#endif //TARGET_DEVICE == RTnode

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <rtnet.h>
#include <rtmac.h>
#include <unistd.h>
#include <limits.h>


#if TARGET_DEVICE == Linux
static struct sockaddr_in rec_addr; /**< Struct containing socket address for receiving. */
static struct sockaddr_in sd_addr_b;/**< Struct containing socket address for sending broadcast. */
static struct sockaddr_in sd_addr_l;/**< Struct containing socket address for sending loop-back. */
int get_sock; /**< Socket for receiving packet from RTnet. */
int bcast_sock; /**< Socket for broadcasting packets to RTnet. */

/**
 *	@brief	Setting all required variable for connection.
 *
 *	Function sets global variables responsible for Ethernet communication
 *	(rec_addr, sd_addr_b, sd_addr_l). Moreover here two sockets are created:
 *	get_sock, bcast_sock. First one is for listening incoming pockets, second
 *	one is used for outgoing transfer.
 *
 *	@pre	Nothing.
 *	@post	Changing values of globla variables: rec_addr, sd_addr_b, sd_addr_l, get_sock, bcast_sock.
 */
void connectToRTnet(){
    if ((bcast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        perror("socket");
    if ((get_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            perror("socket");

    memset(&rec_addr, 0, sizeof(rec_addr));
    memset(&sd_addr_b, 0, sizeof(sd_addr_b));
    memset(&sd_addr_l, 0, sizeof(sd_addr_l));

    rec_addr.sin_family = AF_INET;
    rec_addr.sin_port = htons(MOPS_PORT);
    rec_addr.sin_addr.s_addr =  htonl(INADDR_ANY);//inet_addr("10.0.0.0");

    sd_addr_b.sin_family = AF_INET;
    sd_addr_b.sin_port = htons(MOPS_PORT);
    sd_addr_b.sin_addr.s_addr =  inet_addr(IPADDR);

    sd_addr_l.sin_family = AF_INET;
    sd_addr_l.sin_port = htons(MOPS_PORT);
    sd_addr_l.sin_addr.s_addr =  inet_addr(IPADDR_LO);

	if (bind(get_sock, (struct sockaddr*)&rec_addr, sizeof(rec_addr))==-1)
		perror("bind");
}

/**
 * @brief	Sending buffer to RTnet.
 * @param buf	This a buffer containing data which should be send to other MOPS brokers.
 * @param buflen	Length of buffer in bytes for sending.
 *
 * @pre	Nothing.
 * @post Data are send as a broadcast UDP frame into RTnet and also to myself.
 */
void sendToRTnet(uint8_t *buf, int buflen){
	int write = 0;
	socklen_t len = sizeof(sd_addr_b);
    if((write = sendto(bcast_sock, buf, buflen, 0, (struct sockaddr*)&sd_addr_b, len)) < 0)
        perror("sendto");
    if((write = sendto(bcast_sock, buf, buflen, 0, (struct sockaddr*)&sd_addr_l, len)) < 0)
        perror("sendto");
}

/**
 * @brief	Receiving data from RTnet.
 * @param buf Destination where received data will be stored.
 * @param buflen Maximum length of buffer (in bytes) which can be overridden.
 * @return Actual number of overridden bytes in buffer (amount of written bytes).
 *
 * @post buf variable has been filled with incoming data.
 */
int receiveFromRTnet(uint8_t *buf, int buflen){
	int written = 0;
	socklen_t len = sizeof(rec_addr);
	written = recvfrom(get_sock, buf, buflen, 0, (struct sockaddr*)&rec_addr, &len);
	return written;
}

/**
 * @brief Enable starting of some function in separated thread.
 * @param start_routine Pointer to a function which will be  started as a new thread.
 * @param arg Pointer to a struct containing arguments for function which will be started.
 * @return ID of new thread.
 *
 * @post New thread has been started.
 */
pthread_t startNewThread(void *(*start_routine) (void *), void *arg){
	int err;
	pthread_t thread_id;
    err = pthread_create(&thread_id, NULL, start_routine, arg);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));
    return thread_id;
}

/**
 * @brief Initiation o mutex.
 * @param lock Pointer to mutex we want to be initiated.
 * @return 1 if initiation failed, 0 if everything goes fine.
 *
 * @post Pointed mutex if ready to use.
 */
uint8_t mutex_init(pthread_mutex_t *lock){
    if (pthread_mutex_init(lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    return 0;
}

/**
 * @brief Locking mutex.
 * @param lock Pointer to mutex we want to locked.
 *
 * @post Pointed mutex is locked.
 */
void lock_mutex(pthread_mutex_t *lock){
	pthread_mutex_lock(lock);
}

/**
 * @brief Unlocking mutex.
 * @param lock Pointer to mutex we want to unlock.
 *
 * @post Pointed mutex is unlocked and ready for reuse.
 */
void unlock_mutex(pthread_mutex_t *lock){
	pthread_mutex_unlock(lock);
}
#endif //TARGET == LINUX



#if TARGET_DEVICE == RTnode
static xRTnetSockAddr_t rec_addr; /**< Struct containing socket address for receiving. */
static xRTnetSockAddr_t sd_addr_b;/**< Struct containing socket address for sending broadcast. */
static xRTnetSockAddr_t sd_addr_l;/**< Struct containing socket address for sending loop-back. */
xRTnetSocket_t get_sock; /**< Socket for receiving packet from RTnet. */
xRTnetSocket_t bcast_sock; /**< Socket for broadcasting packets to RTnet. */

void startNewThread(void *(*start_routine) (void *), void *arg){
	TaskHandle_t xHandle = NULL;
	xTaskCreate( (*start_routine), NULL, 400, arg, 3, &xHandle );
}

void connectToRTnet(){
    uint8_t  macBroadcast[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint32_t ip;

	while(xRTnetWaitRedy(portMAX_DELAY) == pdFAIL){;}

	 /* network byte order ip */
    ip  = ulRTnetGetIpAddr();
    /* Add broadcast address */
    ip |= rtnet_htonl(RTNET_NETMASK_BROADCAST);
    xRTnetRouteAdd(macBroadcast, ip);


    if ((bcast_sock = xRTnetSocket(RTNET_AF_INET, RTNET_SOCK_DGRAM, RTNET_IPPROTO_UDP)) == NULL)
        vTaskSuspend(NULL);
    if ((get_sock = xRTnetSocket(RTNET_AF_INET, RTNET_SOCK_DGRAM, RTNET_IPPROTO_UDP)) == NULL)
        vTaskSuspend(NULL);

    rec_addr.sin_port = rtnet_htons(MOPS_PORT);
    rec_addr.sin_addr =  rtnet_htonl(0x00000000UL);//inet_addr("10.0.0.0");

    sd_addr_b.sin_port = rtnet_htons(MOPS_PORT);
    sd_addr_b.sin_addr =  ip;

    sd_addr_l.sin_port = rtnet_htons(MOPS_PORT);
    sd_addr_l.sin_addr = rtnet_htons(IPADDR_LO);

	if (xRTnetBind(get_sock, &rec_addr, sizeof(rec_addr)) != 0)
		vTaskSuspend(NULL);
	rtprintf("Podlaczony, niby\r\n");
}

void sendToRTnet(uint8_t *buf, int buflen){
	int write = 0;
	uint32_t len =  sizeof(sd_addr_b);
    if( (write = lRTnetSendto(bcast_sock, buf, buflen, RTNET_ZERO_COPY, &sd_addr_b, len)) <= 0 )
        perror("sendto");
    if( (write = lRTnetSendto(bcast_sock, buf, buflen, RTNET_ZERO_COPY, &sd_addr_l, len)) <= 0 )
        perror("sendto");
	rtprintf("Wyslane, niby\r\n");
}
int receiveFromRTnet(uint8_t *buf, int buflen){
	int written = 0;
	uint32_t len = sizeof(rec_addr);

	written = lRTnetRecvfrom(get_sock, buf, (size_t) buflen, RTNET_ZERO_COPY, &rec_addr,  &len);
	rtprintf("Odebrane, niby\r\n");
	return written;
}

uint8_t mutex_init(SemaphoreHandle_t *lock){
	*lock = xSemaphoreCreateMutex();
	if( *lock == NULL )
		return 1;
    return 0;
}

void lock_mutex(SemaphoreHandle_t *lock){
	while( xSemaphoreTake( *lock, ( TickType_t ) 0 ) != pdTRUE )
	{;}
}
void unlock_mutex(SemaphoreHandle_t *lock){
	xSemaphoreGive( *lock );
}
#endif //TARGET == RTNODE


//***************** MOPS - MOPS communication protocol ********************
/**
 * @brief Function for creating MOPS protocol "Topic Request" header.
 * "Topic Request" header shape looks like:
 * <pre>
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|1|  <- Topic Request type: 1
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|0|  <- Remaining header length most significant byte: 0
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|0|  <- Remaining header length least significant byte: 0
 * +-+-+-+-+-+-+-+-+
 * </pre>
 * @param Buffer Buffer where header will be stored.
 * @param BufferLen Maximal buffer length which can be overridden.
 * @return Length of buffer (in bytes) which has been overridden - number of written bytes.
 *
 * @post Only if BufferLen is bigger or equal to header length, Buffer contains written header.
 */
uint16_t buildTopicRequestMessage(uint8_t *Buffer, int BufferLen){
	MOPSHeader MHeader;
	uint8_t index = 0;

	MHeader.MOPSMessageType = TOPIC_REQUEST;
	MHeader.RemainingLengthLSB = 0;
	MHeader.RemainingLengthMSB = 0;
	if(BufferLen >= sizeof(MHeader)){
		memcpy(Buffer, &MHeader, sizeof(MHeader));
		index += sizeof(MHeader);
	}
	return index;
}

/**
 * @brief Function for creating MOPS protocol "New Topic" header.
 * "New Topic" header shape looks like:
 * <pre>
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|1|0|  <- Topic Request type: 2
 * +-+-+-+-+-+-+-+-+
 * |X|X|X|X|X|X|X|X|  <- Remaining header length most significant byte.
 * +-+-+-+-+-+-+-+-+
 * |X|X|X|X|X|X|X|X|  <- Remaining header length least significant byte.
 * +-+-+-+-+-+-+-+-+
 * | Topic ID  MSB |  <- Topic identification number most significant byte.
 * +-+-+-+-+-+-+-+-+
 * | Topic ID  LSB |  <- Topic identification number least significant byte.
 * +-+-+-+-+-+-+-+-+
 * | Topic Len MSB |  <- Topic length most significant byte.
 * +-+-+-+-+-+-+-+-+
 * | Topic Len LSB |  <- Topic length least significant byte.
 * +-+-+-+-+-+-+-+-+
 * |X|X|X|X|X|X|X|X|  <- First character of topic string.
 * +-+-+-+-+-+-+-+-+
 *      ......
 * +-+-+-+-+-+-+-+-+
 * |X|X|X|X|X|X|X|X|  <- Last character of topic string..
 * +-+-+-+-+-+-+-+-+
 * </pre>
 * @param Buffer Buffer where header will be stored.
 * @param BufferLen Maximal buffer length which can be overridden.
 * @param Topics Pointer to array containing topics in string format.
 * @param IDs Pointer to array containing topics IDs.
 * @param TopicNo Number of topics in array that we should send as new topic in RTnet.
 * @return Length of buffer (in bytes) which has been overridden - number of written bytes.
 *
 * @post Only if BufferLen is bigger or equal to header length, Buffer contains written header.
 */
uint16_t buildNewTopicMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topics, uint16_t *IDs, int TopicNo){
	MOPSHeader MHeader;
	uint8_t MSB_temp, LSB_temp;
	int index = 0, tempLen = 0, i;
	index = sizeof(MHeader);

	//**** Checking if we have enough space ****//
	tempLen += sizeof(MHeader);
	for (i=0; i<TopicNo; i++)
		tempLen += strlen((char*)(Topics[i])) + 2 + 2;
	if(BufferLen <= tempLen)
		return 0;
	tempLen = 0;
	//**** Checking if we have enough space ****//

	//**** Payload part *****//
	for (i=0; i<TopicNo; i++){
		u16ToMSBandLSB(IDs[i], &MSB_temp, &LSB_temp);
		Buffer[index] = MSB_temp;
		Buffer[index+1] = LSB_temp;
		tempLen = strlen((char*)(Topics[i]));
		u16ToMSBandLSB(tempLen, &MSB_temp, &LSB_temp);
		Buffer[index+2] = MSB_temp;
		Buffer[index+3] = LSB_temp;
		index += 4;
		memcpy(Buffer + index, Topics[i], tempLen);
		index += tempLen;
	}
	//**** Payload part *****//

	u16ToMSBandLSB(index-sizeof(MHeader), &MSB_temp, &LSB_temp);
	MHeader.MOPSMessageType = NEW_TOPICS;
	MHeader.RemainingLengthMSB = MSB_temp;
	MHeader.RemainingLengthLSB = LSB_temp;
	memcpy(Buffer, &MHeader, sizeof(MHeader));

	return (uint16_t)index;
}

/**
 * @brief Function for creating MOPS protocol "Nothing" header.
 * "Nothing" header shape looks like:
 * <pre>
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|1|1|  <- Topic Request type: 3
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|0|  <- Remaining header length most significant byte: 0
 * +-+-+-+-+-+-+-+-+
 * |0|0|0|0|0|0|0|0|  <- Remaining header length least significant byte: 0
 * +-+-+-+-+-+-+-+-+
 * </pre>
 * @param Buffer Buffer where header will be stored.
 * @param BufferLen Maximal buffer length which can be overridden.
 * @return Length of buffer (in bytes) which has been overridden - number of written bytes.
 *
 * @post Only if BufferLen is bigger or equal to header length, Buffer contains written header.
 */
uint16_t buildEmptyMessage(uint8_t *Buffer, int BufferLen){
	MOPSHeader MHeader;
	uint8_t index = 0;

	MHeader.MOPSMessageType = NOTHING;
	MHeader.RemainingLengthLSB = 0;
	MHeader.RemainingLengthMSB = 0;
	if(BufferLen >= sizeof(MHeader)){
		memcpy(Buffer, &MHeader, sizeof(MHeader));
		index += sizeof(MHeader);
	}
	return index;
}
//*********************************************************************

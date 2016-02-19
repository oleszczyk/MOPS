/*
 * local_broker.c
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */
#include <sys/select.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <mqueue.h>
#include "MOPS.h"
#include "MQTT.h"
#include "MOPS_RTnet_Con.h"


static uint8_t MOPS_State = SEND_REQUEST;
uint8_t input_buffer[UDP_MAX_SIZE], output_buffer[UDP_MAX_SIZE], waiting_output_buffer[UDP_MAX_SIZE], waiting_input_buffer[UDP_MAX_SIZE];
uint16_t input_index = 0, output_index = 0, waiting_output_index = 0, waiting_input_index = 0;

TopicID list[MAX_NUMBER_OF_TOPIC];
SubscriberList sub_list[MAX_NUMBER_OF_SUBSCRIPTIONS];
MOPS_Queue mops_queue[MAX_PROCES_CONNECTION];

#if TARGET_DEVICE == Linux
pthread_mutex_t output_lock, input_lock, waiting_output_lock, waiting_input_lock;
#endif
#if TARGET_DEVICE == RTnode
SemaphoreHandle_t output_lock, input_lock, waiting_output_lock, waiting_input_lock;
#endif


int main(void)
{
	int RTsocket = 0;
	mutex_init(&input_lock);
	mutex_init(&output_lock);
	mutex_init(&waiting_output_lock);
	mutex_init(&waiting_input_lock);

	InitTopicList(list);
	MOPS_QueueInit(mops_queue);
	SubListInit(sub_list);
	RTsocket = connectToRTnet();

    startNewThread(&threadSendToRTnet, (void *)RTsocket);
    startNewThread(&threadRecvFromRTnet, (void *)RTsocket);

	InitProcesConnection();
}

void MOPS_QueueInit(MOPS_Queue *queue){
	int i = 0;
	for(i=0; i<MAX_PROCES_CONNECTION; i++)
	{
		queue->MOPSToProces_fd = 0;
		queue->ProcesToMOPS_fd = 0;
	}
}

void SubListInit(SubscriberList *sublist){
	int i = 0;
	for(i=0; i<MAX_NUMBER_OF_SUBSCRIPTIONS; i++){
		sublist[i].ClientID = -1;
		memset(sublist[i].Topic, 0, MAX_TOPIC_LENGTH+1);
	}
}

void threadRecvFromRTnet(int RTsocket){
    for(;;){
    	lock_mutex(&input_lock);
    	input_index = receiveFromRTnet(RTsocket, input_buffer, UDP_MAX_SIZE);
		AnalyzeIncomingUDP(input_buffer, input_index);
		memset(input_buffer, 0, UDP_MAX_SIZE);
		unlock_mutex(&input_lock);
    }
}

void threadSendToRTnet(int RTsocket){
	uint8_t are_local_topics = 0;
	for(;;){
		usleep(10);  // slot czasowy

		switch(MOPS_State){
		case SEND_NOTHING:
			//check if there are local topic to announce
			//if yes, then add them to head of message and update TopicList - reset LocalTopic flag
			//else, send 'nothing'
			are_local_topics = ApplyIDtoNewTopics();
			MoveWaitingToFinal();
			if(are_local_topics)
				SendLocalTopics(list);
			else
				SendEmptyMessage();
			break;
		case SEND_REQUEST:
			SendTopicRequestMessage();
			break;
		case SEND_TOPIC_LIST:
			ApplyIDtoNewTopics();
			MoveWaitingToFinal();
			SendTopicList(list);
			break;
		}

		lock_mutex(&output_lock);
		if ( (output_index > sizeof(MOPSHeader)) || (output_buffer[0] == TOPIC_REQUEST) ){
			sendToRTnet(RTsocket, output_buffer, output_index);
			MOPS_State = SEND_NOTHING;
		}
		memset(output_buffer, 0, UDP_MAX_SIZE);
		output_index = 0;

		unlock_mutex(&output_lock);
	}
}

uint16_t SendEmptyMessage(){
	uint8_t tempLen = 0;
	uint16_t writtenBytes = 0;
	tempLen += sizeof(MOPSHeader);
	if ( tempLen > (UDP_MAX_SIZE-output_index) )
		printf("Not enough space to send Empty Header\n");

	lock_mutex(&output_lock);
	memmove(output_buffer+tempLen, output_buffer, output_index); //Move all existing data
	writtenBytes = buildEmptyMessage(output_buffer, UDP_MAX_SIZE-output_index);
	output_index += writtenBytes;
	unlock_mutex(&output_lock);
	return writtenBytes;
}

uint16_t SendTopicRequestMessage(){
	uint8_t tempLen = 0;
	uint16_t writtenBytes = 0;
	tempLen += sizeof(MOPSHeader);
	if ( tempLen > (UDP_MAX_SIZE-output_index) )
		printf("Not enough space to send Topic Request\n");

	lock_mutex(&output_lock);
	memmove(output_buffer+tempLen, output_buffer, output_index); //Move all existing data
	writtenBytes = buildTopicRequestMessage(output_buffer, UDP_MAX_SIZE-output_index);
	output_index += writtenBytes;
	unlock_mutex(&output_lock);
	return writtenBytes;
}

/*
 * Sending all available (not candidate) topics to RTnet,
 * after that local topics become global.
 */
uint16_t SendTopicList(TopicID list[]){
	int i = 0, counter = 0, tempLen;
	uint8_t *tempTopicList[MAX_NUMBER_OF_TOPIC];
	uint16_t tempTopicIDs[MAX_NUMBER_OF_TOPIC];
	uint16_t writtenBytes;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (list[i].ID != 0){
			tempTopicList[counter] = &list[i].Topic;
			tempTopicIDs[counter] = list[i].ID;
			if(list[i].LocalTopic == 1)
				list[i].LocalTopic = 0;
			counter++;
		}
	}
	tempLen = sizeof(MOPSHeader);
	for (i=0; i<counter; i++)
		tempLen += 2 + 2 + strlen(tempTopicList[i]); //2 for ID msb, ID lsb, 2 for length msb, length lsb.
	if ( tempLen > (UDP_MAX_SIZE-output_index) )
		printf("Not enough space to send all Topics from list\n");

	lock_mutex(&output_lock);
	memmove(output_buffer+tempLen, output_buffer, output_index); //Move all existing data
	writtenBytes = buildNewTopicMessage(output_buffer, UDP_MAX_SIZE-output_index, tempTopicList, tempTopicIDs, counter);
	output_index += writtenBytes;
	unlock_mutex(&output_lock);
	return writtenBytes;
}

/*
 * Sending only local topics to RTnet,
 * after that local topics become global.
 */
uint16_t SendLocalTopics(TopicID list[]){
	int i = 0, counter = 0, tempLen;
	uint8_t *tempTopicList[MAX_NUMBER_OF_TOPIC];
	uint16_t tempTopicIDs[MAX_NUMBER_OF_TOPIC];
	uint16_t writtenBytes;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (list[i].ID != 0 && list[i].LocalTopic==1){
			tempTopicList[counter] = &list[i].Topic;
			tempTopicIDs[counter] = list[i].ID;
			list[i].LocalTopic = 0;
			counter++;
		}
	}

	tempLen = sizeof(MOPSHeader);
	for (i=0; i<counter; i++)
		tempLen += 2 + 2 + strlen(tempTopicList[i]); //2 for ID msb, ID lsb, 2 for length msb, length lsb.
	if ( tempLen > (UDP_MAX_SIZE-output_index) )
		printf("Not enough space to send local Topics from list\n");

	lock_mutex(&output_lock);
	memmove(output_buffer+tempLen, output_buffer, output_index); //Move all existing data
	writtenBytes = buildNewTopicMessage(output_buffer, UDP_MAX_SIZE-output_index, tempTopicList, tempTopicIDs, counter);
	output_index += writtenBytes;
	unlock_mutex(&output_lock);
	return writtenBytes;
}


uint8_t AddTopicToList(TopicID list[], uint8_t *topic, uint16_t topicLen, uint16_t id){
	int i = 0;
	uint16_t tempTopicLength;
	tempTopicLength = (topicLen<MAX_TOPIC_LENGTH) ? topicLen : MAX_TOPIC_LENGTH;

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		//if candidate, apply ID
		if( strncmp(list[i].Topic, topic, tempTopicLength)==0 && list[i].Topic[0]!=0 && list[i].ID==0 ){
			list[i].ID = id;
			//printf("Dodalem ID kandydatowi: %s \n", list[i].Topic);
			return 0;
		}
		// if exists such topic (or at least ID) available, do not do anything
		if ( (list[i].ID == id) || (strncmp(list[i].Topic, topic, tempTopicLength)==0 && list[i].Topic[0]!=0) ){
			//printf("Nie dodam bo jest: %s \n", list[i].Topic);
			return 2;
		}
	}

	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		//else add new topic in the first empty place
		if ( list[i].ID==0 && strlen(list[i].Topic)==0 ){
			memcpy(list[i].Topic, topic, tempTopicLength);
			//printf("Dodany: %s \n", list[i].Topic);
			list[i].ID = id;
			return 0;
		}
	}
	//there is no place in TopicList
	return 1;
}


uint8_t ApplyIDtoNewTopics(){
	int i;
	uint8_t localTopicFlag = 0;
	uint16_t max = 0;

	lock_mutex(&output_lock);
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if(list[i].ID > max)
			max = list[i].ID;
	}
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if ( list[i].ID==0 && strlen(list[i].Topic)!=0 ){
			list[i].ID = max+1;
			list[i].LocalTopic = 1;
			max++;
			localTopicFlag = 1;
		}
	}
	unlock_mutex(&output_lock);
	return localTopicFlag;
}

void AddTopicCandidate(uint8_t *topic, uint16_t topicLen){
	int i;
	uint16_t tempTopicLength;

	tempTopicLength = (topicLen<MAX_TOPIC_LENGTH) ? topicLen : MAX_TOPIC_LENGTH;
	if(GetIDfromTopicName(topic, tempTopicLength) == -1)
		for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
			if ( list[i].ID==0 && strlen(list[i].Topic)==0 ){
				memcpy(list[i].Topic, topic, tempTopicLength);
				return;
			}
		}
}

/*
 * return:
 *  ID (uint16_t value) if topic exist already in TopicList and is available
 *  0					if topic is candidate in TopicList
 *  -1					if topic is not available, and not candidate
 */
int GetIDfromTopicName(uint8_t *topic, uint16_t topicLen){
	int i;
	uint16_t tempTopicLength;

	tempTopicLength = (topicLen<MAX_TOPIC_LENGTH) ? topicLen : MAX_TOPIC_LENGTH;
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (strncmp(list[i].Topic, topic, tempTopicLength)==0 && list[i].Topic[0]!=0)  //when  are the same
				return list[i].ID;
	}
	return -1;
}

/*
 * POST: variable 'topic' is set as Topic with id 'id',
 * if there is not a topic in TopicList with that id
 * variable 'topic' is set to \0.
 */
uint16_t GetTopicNameFromID(uint16_t id, uint8_t *topic){
	int i;
	uint16_t len = 0;

	memset(topic, 0, MAX_TOPIC_LENGTH+1);
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		if (list[i].ID == id){  //when  are the same
			len = strlen(list[i].Topic);
			memcpy(topic, &list[i].Topic, len);
			return len;
		}
	}
	return 0;
}

void InitTopicList(TopicID list[]){
	int i = 0;
	for (i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		list[i].ID = 0;
		list[i].LocalTopic = 0;
		memset(&list[i].Topic, 0, MAX_TOPIC_LENGTH+1);
	}
}

void PrintfList(TopicID list[]){
	int i;
	printf("Lista{\n");
	for(i=0; i<MAX_NUMBER_OF_TOPIC; i++){
		printf("    Topic: %s, ID: %d \n", list[i].Topic, list[i].ID);
	}
	printf("};\n");
}

void PrintfSubList(SubscriberList sublist[]){
	int i;
	printf("SubList{\n");
	for(i=0; i<MAX_NUMBER_OF_SUBSCRIPTIONS; i++){
		printf("    Topic: %s, SubscriberID: %d \n", sublist[i].Topic, sublist[i].ClientID);
	}
	printf("};\n");
}

void AnalyzeIncomingUDP(uint8_t *Buffer, int written_bytes){
	MOPSHeader MHeader;
	uint16_t MOPSMessageLen;
	uint8_t HeadLen = sizeof(MHeader);

	memcpy(&MHeader, Buffer, HeadLen);
	MOPSMessageLen = MSBandLSBTou16(MHeader.RemainingLengthMSB, MHeader.RemainingLengthLSB) + HeadLen;

	switch(MHeader.MOPSMessageType){
	case TOPIC_REQUEST:
		lock_mutex(&output_lock);
		MOPS_State = SEND_TOPIC_LIST;
		unlock_mutex(&output_lock);
		break;
	case NEW_TOPICS:
		lock_mutex(&output_lock);
		UpdateTopicList(Buffer, written_bytes);
		unlock_mutex(&output_lock);
		break;
	case NOTHING:
		//do not change state
		break;
	}
	//Move remaining data to buffer beginning
	lock_mutex(&waiting_input_lock);
	if( (UDP_MAX_SIZE-waiting_input_index)>=(written_bytes-MOPSMessageLen) ){ //If we have enough space
		memmove(waiting_input_buffer+waiting_input_index, Buffer+MOPSMessageLen, written_bytes-MOPSMessageLen);
		waiting_input_index += (written_bytes-MOPSMessageLen);
	}
	unlock_mutex(&waiting_input_lock);
}

void UpdateTopicList(uint8_t *Buffer, int BufferLen){
	uint16_t index = 0, messageLength = 0;
	uint16_t tempTopicLength = 0, tempTopicID = 0;
	uint8_t err;

	messageLength = MSBandLSBTou16(Buffer[1], Buffer[2]) + 3;
	index += 3;
	for(; index<messageLength; ){
		tempTopicID = MSBandLSBTou16(Buffer[index], Buffer[index+1]);
		tempTopicLength = MSBandLSBTou16(Buffer[index+2], Buffer[index+3]);
		index += 4;

		err = AddTopicToList(list, Buffer+index, tempTopicLength, tempTopicID);
		index += tempTopicLength;
		/*if(err == 1)
			printf("Brak miejsca na liscie! \n");
		if(err == 0)
			printf("Dodalem, id: %d \n", tempTopicID);
		if(err == 2)
			printf("Topic, id: %d, juz istnieje. \n", tempTopicID);
		*/
	}
}

int AddToMOPSQueue(int MOPS_Proces_fd, int Proces_MOPS_fd){
	int i = 0;
	for(i=0; i<MAX_PROCES_CONNECTION; i++)
		if(mops_queue[i].MOPSToProces_fd==0 && mops_queue[i].ProcesToMOPS_fd==0){
			mops_queue[i].MOPSToProces_fd = MOPS_Proces_fd;
			mops_queue[i].ProcesToMOPS_fd = Proces_MOPS_fd;
			return i;
		}
	return -1;
}

#if TARGET_DEVICE == Linux
void InitProcesConnection(){
    mqd_t mq_listener, new_mq_Proces_MOPS;
    struct mq_attr attr;
    struct timeval tv;
    int bytes_read, fdmax, rv, i;
    fd_set master, read_fd;  //master fd list, temp fd list for select()
	FD_ZERO(&master);
	FD_ZERO(&read_fd);

    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_QUEUE_SIZE;
    attr.mq_curmsgs = 0;

	mq_listener = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if( !((mqd_t)-1 != mq_listener) )
    	perror("MQueue Open listener");

    FD_SET(mq_listener, &master);
    fdmax = mq_listener;
    for (;;){
    	tv.tv_sec = 0;
    	tv.tv_usec = 1;
    	read_fd = master;
    	rv = select(fdmax+1, &read_fd, NULL, NULL, &tv);
    	if(rv > 0){		// there are file descriptors to serve
    		for(i = 0; i <=fdmax; i++){
    			if (FD_ISSET(i, &read_fd)){
					if(i == mq_listener){
						new_mq_Proces_MOPS = ServeNewProcessConnection(&master, mq_listener);
						if(new_mq_Proces_MOPS > fdmax)
							fdmax = new_mq_Proces_MOPS;
					}
					else{
						ReceiveFromProcess(i);
					}
    			}
    		}
    	}
    	if(rv < 0)		// error occurred in select()
    	    perror("select");
    	if(rv == 0)		// timeout, we can do our things
    		ServeSendingToProcesses();
    }
}

int ReceiveFromProcess(int file_de){
	int bytes_read, ClientID;
    uint8_t temp[MAX_QUEUE_SIZE+1];

	bytes_read = mq_receive(file_de, temp, MAX_QUEUE_SIZE, NULL);
	if(bytes_read>=sizeof(FixedHeader)){
		ClientID = FindClientIDbyFileDesc(file_de);
		AnalyzeProcessMessage(temp, bytes_read, ClientID);
	}
	return 0;
}

int SendToProcess(uint8_t *buffer, uint16_t buffLen, int file_de){
	return mq_send(file_de, buffer, buffLen, 0);
}

/*
 * Return:
 * 	file descriptor (int) - when there is place in MOPSQueue array
 * 	-1 					  - if there is not place in MOPSQueue array or no message received from listener_fd
 */
int ServeNewProcessConnection(fd_set *set, int listener_fd){
    struct mq_attr attr;
    uint8_t buffer[MAX_QUEUE_SIZE+1], temp;
    int new_mq_Proces_MOPS, new_mq_MOPS_Proces;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_QUEUE_SIZE;
    attr.mq_curmsgs = 0;
    memset(buffer, 0, MAX_QUEUE_SIZE+1);

    if(mq_receive(listener_fd, buffer, MAX_QUEUE_SIZE, NULL) > 0){
    	temp = strlen(buffer);
    	buffer[temp] = 'b';
    	new_mq_Proces_MOPS = mq_open(buffer, O_RDONLY);
		if( !((mqd_t)-1 != new_mq_Proces_MOPS) )
			perror("MQueue Open Proces_MOPS");

    	buffer[temp] = 'a';
		new_mq_MOPS_Proces = mq_open(buffer, O_WRONLY);
		if( !((mqd_t)-1 != new_mq_MOPS_Proces) )
			perror("MQueue Open MOPS_Proces");

		if (AddToMOPSQueue(new_mq_MOPS_Proces, new_mq_Proces_MOPS) >= 0){
			FD_SET(new_mq_Proces_MOPS, set);
			//printf("Nowy deskryptor: %d, nazwa kolejki: %s \n", new_mq_Proces_MOPS, buffer);
			return new_mq_Proces_MOPS;
		}
    }
    return -1;
}
#endif //TARGET_DEVICE == Linux


//TODO
#if TARGET_DEVICE == RTnode
void InitProcesConnection(){

	for(;;){}
}
#endif //TARGET_DEVICE == RTnode

int ServeSendingToProcesses(){
	uint8_t tempBuffer[UDP_MAX_SIZE], HeadLen;
	uint16_t FrameLen = 0, OldFrameLen = 0, written_bytes = 0;
	FixedHeader FHeader;
	memset(tempBuffer, 0, UDP_MAX_SIZE);

	lock_mutex(&waiting_input_lock);
	if(waiting_input_index > 0){
		written_bytes = waiting_input_index;
		memcpy(tempBuffer, waiting_input_buffer, waiting_input_index);
		memset(waiting_input_buffer, 0 , UDP_MAX_SIZE);
		waiting_input_index = 0;
	}
	unlock_mutex(&waiting_input_lock);

	if(written_bytes>0){
		HeadLen = sizeof(FHeader);
		memcpy(&FHeader, tempBuffer + FrameLen, HeadLen);
		FrameLen += MSBandLSBTou16(FHeader.RemainingLengthMSB, FHeader.RemainingLengthLSB) + HeadLen;
		while(FHeader.MessageType!=0 && FrameLen<=written_bytes)
		{
			PrepareFrameToSendToProcess(tempBuffer+OldFrameLen, FrameLen-OldFrameLen);

			memcpy(&FHeader, tempBuffer + FrameLen, HeadLen);
			OldFrameLen = FrameLen;
			FrameLen += MSBandLSBTou16(FHeader.RemainingLengthMSB, FHeader.RemainingLengthLSB) + HeadLen;
		}
	}
	return 0;
}

void PrepareFrameToSendToProcess(uint8_t *Buffer, int written_bytes){
	 uint16_t topicID, topicLen, index = 0;
	 uint8_t tempBuffer[MAX_QUEUE_SIZE], HeaderLen;
	 uint8_t tempTopic[MAX_TOPIC_LENGTH+1], i, tempMSB = 0, tempLSB = 0;
	 FixedHeader FHeader;
	 int clientID;

	 memset(tempBuffer, 0, MAX_QUEUE_SIZE);
	 HeaderLen = sizeof(FHeader);

	 topicID = MSBandLSBTou16(Buffer[HeaderLen], Buffer[HeaderLen+1]);
	 topicLen = GetTopicNameFromID(topicID, tempTopic);
	 clientID = FindClientIDbyTopic(tempTopic, topicLen);
	 u16ToMSBandLSB(topicLen, &tempMSB, &tempLSB);

	 Buffer[ HeaderLen ] = tempMSB;
	 Buffer[HeaderLen+1] = tempLSB;
	 index = HeaderLen+2;
	 memmove(Buffer+index+topicLen, Buffer+index, written_bytes-index);
	 memcpy(Buffer+index, tempTopic, topicLen);

	 SendToProcess(Buffer, written_bytes+topicLen, mops_queue[clientID].MOPSToProces_fd);
}

int FindClientIDbyTopic(uint8_t *topic, uint16_t topicLen){
	int i;
	for(i=0; i<MAX_NUMBER_OF_SUBSCRIPTIONS; i++){
		if(strncmp(sub_list[i].Topic, topic, topicLen) == 0){
			return sub_list[i].ClientID;
		}
	}
	return -1;
}

int FindClientIDbyFileDesc(int file_de){
	int i = 0;
	for(i=0; i<MAX_NUMBER_OF_SUBSCRIPTIONS; i++)
		if( mops_queue[i].MOPSToProces_fd==file_de || mops_queue[i].ProcesToMOPS_fd==file_de)
			return i;
	return -1;
}

void AnalyzeProcessMessage(uint8_t *buffer, int bytes_wrote, int ClientID){
	FixedHeader FHeader;
	uint8_t HeadLen = 0;
	uint16_t FrameLen = 0, OldFrameLen = 0;
	HeadLen = sizeof(FHeader);

	memcpy(&FHeader, buffer + FrameLen, HeadLen);
	FrameLen += MSBandLSBTou16(FHeader.RemainingLengthMSB, FHeader.RemainingLengthLSB) + HeadLen;
	while(FHeader.MessageType!=0 && FrameLen<=bytes_wrote)
	{
		switch(FHeader.MessageType){
		case PUBLISH:
			ServePublishMessage(buffer+OldFrameLen, FrameLen-OldFrameLen);
			break;
		case SUBSCRIBE:
			ServeSubscribeMessage(buffer+OldFrameLen, FrameLen-OldFrameLen, ClientID);
			break;
		}
		memcpy(&FHeader, buffer + FrameLen, HeadLen);
		OldFrameLen = FrameLen;
		FrameLen += MSBandLSBTou16(FHeader.RemainingLengthMSB, FHeader.RemainingLengthLSB) + HeadLen;
	}
}

void ServePublishMessage(uint8_t *buffer, int FrameLen){
	uint8_t topicTemp[MAX_TOPIC_LENGTH+1];
	uint16_t TopicLen, index = 0;
	int topicID;
	memset(topicTemp, 0, MAX_TOPIC_LENGTH+1);

	index+=3;
	TopicLen = MSBandLSBTou16(buffer[index], buffer[index+1]);
	index+=2;
	memcpy(topicTemp, buffer+index, TopicLen);
	index+=TopicLen;

	topicID = GetIDfromTopicName(topicTemp, TopicLen);
	switch(topicID){
	case -1:
		AddTopicCandidate(topicTemp, TopicLen);
		AddPacketToWaitingTab(buffer, FrameLen);
		break;
	case 0:
		AddPacketToWaitingTab(buffer, FrameLen);
		break;
	default:
		AddPacketToFinalTab(buffer, FrameLen, topicID);
		break;
	}
}

void ServeSubscribeMessage(uint8_t *buffer, int FrameLen, int ClientID){
	uint16_t TopicLen, index = 0;

	index+=5;
	do{
		TopicLen = MSBandLSBTou16(buffer[index], buffer[index+1]);
		index+=2;
		AddToSubscribersList(buffer+index, TopicLen, ClientID);
		index+=(TopicLen+1);
	}while(index<FrameLen);
}

int AddToSubscribersList(uint8_t *topic, uint16_t topicLen, int ClientID){
	int i = 0;
	uint16_t tempTopicLen;

	for(i=0; i<MAX_NUMBER_OF_SUBSCRIPTIONS; i++){
		if(sub_list[i].ClientID==ClientID && strncmp(sub_list[i].Topic, topic, topicLen)==0 && sub_list[i].Topic[0]!=0){
			return -1; //This subscription for that client already exists
		}
	}
	tempTopicLen = (topicLen < MAX_TOPIC_LENGTH) ? topicLen : MAX_TOPIC_LENGTH;
	for(i=0; i<MAX_NUMBER_OF_SUBSCRIPTIONS; i++){
		if(sub_list[i].ClientID == -1){
			memcpy(sub_list[i].Topic, topic, tempTopicLen);
			sub_list[i].ClientID = ClientID;
			return i; //Subscription has been added successfully
		}
	}
	return 0; //There is no place to store subscription!
}

void AddPacketToWaitingTab(uint8_t *buffer, int FrameLen){
	lock_mutex(&waiting_output_lock);
	memcpy(waiting_output_buffer+waiting_output_index, buffer, FrameLen);
	waiting_output_index += FrameLen;
	unlock_mutex(&waiting_output_lock);
}

void AddPacketToFinalTab(uint8_t *buffer, int FrameLen, uint16_t topicID){
	uint8_t tempBuff[MAX_QUEUE_SIZE];
	uint8_t MSBtemp, LSBtemp, headLen, index = 0;
	uint16_t TopicLen, MessageLen;
	memset(tempBuff,0,MAX_QUEUE_SIZE);

	headLen = sizeof(FixedHeader);
	u16ToMSBandLSB(topicID, &MSBtemp, &LSBtemp);
	memcpy(tempBuff, buffer, headLen);
	MessageLen = MSBandLSBTou16(buffer[1], buffer[2]);

	tempBuff[ headLen ] = MSBtemp;
	tempBuff[headLen+1] = LSBtemp;
	index = headLen+2;

	TopicLen = MSBandLSBTou16(buffer[headLen], buffer[headLen+1]);
	MessageLen = MessageLen - TopicLen;
	u16ToMSBandLSB(MessageLen, &MSBtemp, &LSBtemp);
	tempBuff[1] = MSBtemp; //New message len MSB
	tempBuff[2] = LSBtemp; //New message len LSB

	memcpy( tempBuff+index, buffer+index+TopicLen, FrameLen-(index+TopicLen) );

	lock_mutex(&output_lock);
	memcpy(output_buffer+output_index, tempBuff, FrameLen-TopicLen);
	output_index += (FrameLen-TopicLen);
	//printf("Frame: %d, TopicLen: %d \n", FrameLen, TopicLen);
	unlock_mutex(&output_lock);
}

void MoveWaitingToFinal(){
	uint8_t tempTab[UDP_MAX_SIZE];
	uint16_t tempIndex = 0;

	lock_mutex(&waiting_output_lock);
	memcpy(tempTab, waiting_output_buffer, waiting_output_index);
	memset(waiting_output_buffer, 0 , UDP_MAX_SIZE);
	tempIndex = waiting_output_index;
	waiting_output_index = 0;
	unlock_mutex(&waiting_output_lock);

	AnalyzeProcessMessage(tempTab, tempIndex, -1);
}

void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB){
	uint16_t temp;
	*LSB = (uint8_t) u16bit;
	temp = u16bit>>8;
	*MSB = (uint8_t) temp;
}

uint16_t MSBandLSBTou16(uint8_t MSB, uint8_t LSB){
	uint16_t temp;
	temp = MSB;
	temp = temp<<8;
	temp += LSB;
	return temp;
}

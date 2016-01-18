/*
 * MQTT.c
 *
 *  Created on: Jan 16, 2016
 *      Author: rudy
 */
#include <string.h>
#include <time.h>

#include "MQTT.h"
#include "MQTTConf.h"


/* Function return length of applied bytes.
 */
uint16_t BuildConnectMessage(uint8_t *Message, int MessageLen, uint16_t KeepAlive){
	FixedHeader FHeader;
	ConnectVariableHeader CVHeader;
	uint8_t i, MSB_temp, LSB_temp, flags;
	uint16_t length;
	int index;
	int tempLen = 0;

	flags = (CLEANSESSION<<1) + (WILLFLAG<<2) + (WILLQOS<<3) + (WILLRETAIN<<5) + (PASSWORDFLAG<<6) + (USERNAMEFLAG<<7);


	Init_FixedHeader(&FHeader, CONNECT, 0);
	Init_ConnectVariableHeader(&CVHeader, flags, KeepAlive);

	memset(Message, 0, MessageLen);

	//Check if all data can be stored in Message buffer
	tempLen += sizeof(FHeader) + sizeof(CVHeader) + 2 + sizeof(CLIENTID)-1;
#if WILLFLAG == 1
	tempLen += 2 + sizeof(WILLTOPIC)-1;
#endif
#if USERNAMEFLAG == 1
	tempLen += 2 + sizeof(USERNAME)-1;
#endif
#if USERNAMEFLAG == 1 && PASSWORDFLAG == 1
	tempLen += 2 + sizeof(PASSWORD)-1;
#endif
	if (tempLen > MessageLen)
		return 0;				//Needed memory is bigger than allocated one.

	index = sizeof(FHeader);
	memcpy(Message+index, &CVHeader, sizeof(CVHeader));
	index += sizeof(CVHeader);

	//**** Payload part *****//
	length = sizeof(CLIENTID)-1;
	u16ToMSBandLSB(length, &MSB_temp, &LSB_temp);
	Message[index] = MSB_temp;
	Message[index+1] = LSB_temp;
	index += 2;
	memcpy(Message+index, CLIENTID, length);
	index += sizeof(CLIENTID)-1;

#if WILLFLAG == 1
	length = sizeof(WILLTOPIC)-1;
	u16ToMSBandLSB(length, &MSB_temp, &LSB_temp);
	Message[index] = MSB_temp;
	Message[index+1] = LSB_temp;
	index += 2;
	memcpy(Message+index, WILLTOPIC, length);
	index += sizeof(WILLTOPIC)-1;

	length = sizeof(WILLMESSAGE)-1;
	u16ToMSBandLSB(length, &MSB_temp, &LSB_temp);
	Message[index] = MSB_temp;
	Message[index+1] = LSB_temp;
	index += 2;
	memcpy(Message+index, WILLMESSAGE, length);
	index += sizeof(WILLMESSAGE)-1;
#endif

#if USERNAMEFLAG == 1
	length = sizeof(USERNAME)-1;
	u16ToMSBandLSB(length, &MSB_temp, &LSB_temp);
	Message[index] = MSB_temp;
	Message[index+1] = LSB_temp;
	index += 2;
	memcpy(Message+index, USERNAME, length);
	index += sizeof(USERNAME)-1;
#endif

#if USERNAMEFLAG == 1 && PASSWORDFLAG == 1
	length = sizeof(PASSWORD)-1;
	u16ToMSBandLSB(length, &MSB_temp, &LSB_temp);
	Message[index] = MSB_temp;
	Message[index+1] = LSB_temp;
	index += 2;
	memcpy(Message+index, PASSWORD, length);
	index += sizeof(PASSWORD)-1;
#endif
	//**** Payload part *****//

	u16ToMSBandLSB(index-sizeof(FixedHeader), &MSB_temp, &LSB_temp);
	FHeader.RemainingLengthMSB = MSB_temp;
	FHeader.RemainingLengthLSB = LSB_temp;
	memcpy(Message, &FHeader, sizeof(FHeader));

	return (uint16_t) index;
}

uint16_t BuildConnACKMessage(uint8_t *Buffer, int BufferLen, uint8_t SessionPresent, uint8_t ReturnCode){
	FixedHeader FHeader;
	int index = 0;
	int tempLen = 0;

	//Check if all data can be stored in Message buffer
	tempLen = sizeof(FHeader) + 2;
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	Init_FixedHeader(&FHeader, CONNACK, 0);
	FHeader.RemainingLengthMSB = 0;
	FHeader.RemainingLengthLSB = 2;

	memcpy(Buffer, &FHeader, sizeof(FHeader));
	index += sizeof(FHeader);
	Buffer[ index ] =  SessionPresent & 0x01;
	Buffer[index+1] =  ReturnCode;
	index += 2;

	//**** Payload part *****//
	//**** Payload part *****//

	return (uint16_t) index;
}

/* Function return length of applied bytes.
 */
uint16_t BuildClientPublishMessage(uint8_t *Buffer, int BufferLen, uint8_t* Topic, uint8_t* Message, uint8_t DUP, uint8_t Retain, uint16_t *packetID){
	FixedHeader FHeader;
	PublishVariableHeader PVHeader;
	uint8_t MSB_temp, LSB_temp, Flags = 0;
	int tempLen = 0, index = 0;
	*packetID = 0;

	if (QOS == 0)
		DUP = 0;
	Flags = (DUP<<3) + (QOS<<1) + Retain;

	//Check if all data can be stored in Message buffer
	tempLen += sizeof(FHeader) + 2 + strlen(Topic) + 2 +strlen(Message);
	if( QOS > 0)
		tempLen += 2;
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	Init_FixedHeader(&FHeader, PUBLISH, Flags);
	Init_TopicName(&PVHeader.PName, Topic);


	if (QOS > 0)//Full fill packet identifiers
		srand ( time(NULL) );
		*packetID = (uint16_t) (rand() % 65535);

	u16ToMSBandLSB(*packetID, &MSB_temp, &LSB_temp);
	PVHeader.MSB_PacketIdentifier = MSB_temp;
	PVHeader.LSB_PacketIdentifier = LSB_temp;

	index = sizeof(FHeader);
	tempLen = (PVHeader.PName.MSB_Length<<8) + PVHeader.PName.LSB_Length;
	Buffer[ index ] = PVHeader.PName.MSB_Length;
	Buffer[index+1] = PVHeader.PName.LSB_Length;
	index+=2;
	memcpy(Buffer + index, PVHeader.PName.Topic, tempLen);
	index += tempLen;

	if (QOS > 0){
		Buffer[index] = PVHeader.MSB_PacketIdentifier;
		Buffer[index+1] = PVHeader.LSB_PacketIdentifier;
		index += 2;
	}

	//**** Payload part *****//
	tempLen = strlen(Message);
	u16ToMSBandLSB(tempLen, &MSB_temp, &LSB_temp);
	Buffer[index] = MSB_temp;
	Buffer[index+1] = LSB_temp;
	index += 2;
	memcpy(Buffer + index, Message, tempLen);
	index += tempLen;
	//**** Payload part *****//

	u16ToMSBandLSB(index-sizeof(FixedHeader), &MSB_temp, &LSB_temp);
	FHeader.RemainingLengthMSB = MSB_temp;
	FHeader.RemainingLengthLSB = LSB_temp;
	memcpy(Buffer, &FHeader, sizeof(FHeader));

	return (uint16_t) index;
}

uint16_t BuildPubACKMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	FixedHeader FHeader;
	PubACKVariableHeader PACKHeader;
	uint8_t MSB_temp, LSB_temp;
	int tempLen = 0, index = 0;

	//Check if all data can be stored in Message buffer
	tempLen = sizeof(FHeader) + sizeof(PACKHeader);
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	Init_FixedHeader(&FHeader, PUBACK, 0);
	FHeader.RemainingLengthMSB = 0;
	FHeader.RemainingLengthLSB = 2;

	u16ToMSBandLSB(packetID, &MSB_temp, &LSB_temp);
	PACKHeader.MSB_PacketIdentifier = MSB_temp;
	PACKHeader.LSB_PacketIdentifier = LSB_temp;

	memcpy(Buffer, &FHeader, sizeof(FHeader));
	index = sizeof(FHeader);
	memcpy(Buffer+index, &PACKHeader, sizeof(PACKHeader));
	index += sizeof(PACKHeader);

	return (uint16_t) index;
}

uint16_t BuildPubRecMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	FixedHeader FHeader;
	PubACKVariableHeader PACKHeader;
	uint8_t MSB_temp, LSB_temp;
	int tempLen = 0, index = 0;

	//Check if all data can be stored in Message buffer
	tempLen = sizeof(FHeader) + sizeof(PACKHeader);
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	Init_FixedHeader(&FHeader, PUBREC, 0);
	FHeader.RemainingLengthMSB = 0;
	FHeader.RemainingLengthLSB = 2;

	u16ToMSBandLSB(packetID, &MSB_temp, &LSB_temp);
	PACKHeader.MSB_PacketIdentifier = MSB_temp;
	PACKHeader.LSB_PacketIdentifier = LSB_temp;

	memcpy(Buffer, &FHeader, sizeof(FHeader));
	index = sizeof(FHeader);
	memcpy(Buffer+index, &PACKHeader, sizeof(PACKHeader));
	index += sizeof(PACKHeader);

	return (uint16_t) index;
}

uint16_t BuildPubRelMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	FixedHeader FHeader;
	PubACKVariableHeader PACKHeader;
	uint8_t MSB_temp, LSB_temp;
	int tempLen = 0, index = 0;

	//Check if all data can be stored in Message buffer
	tempLen = sizeof(FHeader) + sizeof(PACKHeader);
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	Init_FixedHeader(&FHeader, PUBREL, 0);
	FHeader.RemainingLengthMSB = 0;
	FHeader.RemainingLengthLSB = 2;

	u16ToMSBandLSB(packetID, &MSB_temp, &LSB_temp);
	PACKHeader.MSB_PacketIdentifier = MSB_temp;
	PACKHeader.LSB_PacketIdentifier = LSB_temp;

	memcpy(Buffer, &FHeader, sizeof(FHeader));
	index = sizeof(FHeader);
	memcpy(Buffer+index, &PACKHeader, sizeof(PACKHeader));
	index += sizeof(PACKHeader);

	return (uint16_t) index;
}

uint16_t BuildPubCompMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	FixedHeader FHeader;
	PubACKVariableHeader PACKHeader;
	uint8_t MSB_temp, LSB_temp;
	int tempLen = 0, index = 0;

	//Check if all data can be stored in Message buffer
	tempLen = sizeof(FHeader) + sizeof(PACKHeader);
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	Init_FixedHeader(&FHeader, PUBCOMP, 0);
	FHeader.RemainingLengthMSB = 0;
	FHeader.RemainingLengthLSB = 2;

	u16ToMSBandLSB(packetID, &MSB_temp, &LSB_temp);
	PACKHeader.MSB_PacketIdentifier = MSB_temp;
	PACKHeader.LSB_PacketIdentifier = LSB_temp;

	memcpy(Buffer, &FHeader, sizeof(FHeader));
	index = sizeof(FHeader);
	memcpy(Buffer+index, &PACKHeader, sizeof(PACKHeader));
	index += sizeof(PACKHeader);

	return (uint16_t) index;
}

/* Function return length of applied bytes.
 */
uint16_t BuildSubscribeMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topic, uint8_t *QoS, uint8_t TopicNo, uint16_t *packetID){
	FixedHeader FHeader;
	SubscribeVariableHeader SVHeader;
	uint8_t MSB_temp, LSB_temp, i = 0;
	int tempLen = 0, index = 0;

	Init_FixedHeader(&FHeader, SUBSCRIBE, 0);

	srand ( time(NULL) );
	*packetID = (uint16_t) (rand() % 65535);

	//Check if all data can be stored in Message buffer
	tempLen += sizeof(FHeader) + sizeof(SVHeader);
	for (i=0; i<TopicNo; i++)
		tempLen += 2 + strlen(Topic[i]) + 1;
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	u16ToMSBandLSB(*packetID, &MSB_temp, &LSB_temp);
	SVHeader.MSB_PacketIdentifier = MSB_temp;
	SVHeader.LSB_PacketIdentifier = LSB_temp;

	index = sizeof(FHeader);
	memcpy(Buffer+index, &SVHeader, sizeof(SVHeader));
	index += sizeof(SVHeader);

	//**** Payload part *****//
	for (i=0; i<TopicNo; i++){
		tempLen = strlen(Topic[i]);
		u16ToMSBandLSB(tempLen, &MSB_temp, &LSB_temp);
		Buffer[index] = MSB_temp;
		Buffer[index+1] = LSB_temp;
		index += 2;
		memcpy(Buffer + index, Topic[i], tempLen);
		index += tempLen;
		Buffer[index] = QoS[i];
		index += 1;
	}
	//**** Payload part *****//

	u16ToMSBandLSB(index-sizeof(FixedHeader), &MSB_temp, &LSB_temp);
	FHeader.RemainingLengthMSB = MSB_temp;
	FHeader.RemainingLengthLSB = LSB_temp;
	memcpy(Buffer, &FHeader, sizeof(FHeader));

	return (uint16_t) index;
}


uint16_t BuildSubACKMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID, uint8_t *QoSReturnCode, uint8_t TopicNo){
	FixedHeader FHeader;
	SubscribeVariableHeader SVHeader;
	uint8_t MSB_temp, LSB_temp, i = 0;
	int tempLen = 0, index = 0;

	Init_FixedHeader(&FHeader, SUBACK, 0);

	u16ToMSBandLSB(packetID, &MSB_temp, &LSB_temp);
	SVHeader.MSB_PacketIdentifier = MSB_temp;
	SVHeader.LSB_PacketIdentifier = LSB_temp;

	//Check if all data can be stored in Message buffer
	tempLen += sizeof(FHeader) + sizeof(SVHeader);
	for (i=0; i<TopicNo; i++)
		tempLen += 1;
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	index = sizeof(FHeader);
	memcpy(Buffer+index, &SVHeader, sizeof(SVHeader));
	index += sizeof(SVHeader);

	//**** Payload part *****//
	for (i=0; i<TopicNo; i++){
		Buffer[index] = QoSReturnCode[i];
		index += 1;
	}
	//**** Payload part *****//

	u16ToMSBandLSB(index-sizeof(FixedHeader), &MSB_temp, &LSB_temp);
	FHeader.RemainingLengthMSB = MSB_temp;
	FHeader.RemainingLengthLSB = LSB_temp;
	memcpy(Buffer, &FHeader, sizeof(FHeader));

	return (uint16_t) index;
}

uint16_t BuildUnSubACKMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	FixedHeader FHeader;
	SubscribeVariableHeader UnSubACKHeader;
	uint8_t MSB_temp, LSB_temp;
	int tempLen = 0, index = 0;

	//Check if all data can be stored in Message buffer
	tempLen = sizeof(FHeader) + sizeof(UnSubACKHeader);
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	Init_FixedHeader(&FHeader, UNSUBACK, 0);
	FHeader.RemainingLengthMSB = 0;
	FHeader.RemainingLengthLSB = 2;

	u16ToMSBandLSB(packetID, &MSB_temp, &LSB_temp);
	UnSubACKHeader.MSB_PacketIdentifier = MSB_temp;
	UnSubACKHeader.LSB_PacketIdentifier = LSB_temp;

	memcpy(Buffer, &FHeader, sizeof(FHeader));
	index = sizeof(FHeader);
	memcpy(Buffer+index, &UnSubACKHeader, sizeof(UnSubACKHeader));
	index += sizeof(UnSubACKHeader);

	return (uint16_t) index;
}

void gen_random(char *s, const int len) {
	int i = 0;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    for(i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    s[len] = 0;
}


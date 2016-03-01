/*
 * MQTT.c
 *
 *  Created on: Jan 16, 2016
 *      Author: rudy
 */
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "MOPS.h"
#include "MQTT.h"
#include "MQTTConf.h"



void Init_FixedHeader(FixedHeader *FHeader, uint8_t MessageType, uint8_t Flags){
	FHeader->MessageType = MessageType;
	switch(MessageType)
	{
		case PUBREL:
		case SUBSCRIBE:
		case UNSUBSCRIBE:
			FHeader->Flags = 2;
			break;
		case PUBLISH:
			FHeader->Flags = (Flags & 0x0F);
			break;
		default:
			FHeader->Flags = 0;
			break;
	}
}


void Init_ProtocolName(ProtocolName *PName){
	PName->MSB_Length = 0;
	PName->LSB_Length = 4;
	PName->Name[0] = 'M';
	PName->Name[1] = 'Q';
	PName->Name[2] = 'T';
	PName->Name[3] = 'T';
}

void Init_ConnectVariableHeader(ConnectVariableHeader *CVHeader, uint8_t Flags, uint16_t KeepAlive){
	uint8_t MSBtemp, LSBtemp;
	Init_ProtocolName(&CVHeader->PName);
	CVHeader->ProtocolLevel = 4;
	CVHeader->ConnectFlags = Flags;

	u16ToMSBandLSB(KeepAlive, &MSBtemp, &LSBtemp);
	CVHeader->LSB_KeepAlive = LSBtemp;
	CVHeader->MSB_KeepAlive = MSBtemp;
}

void Init_TopicName(TopicName *TName, uint8_t *Topic){
	uint16_t TopicLen = strlen((char*)Topic);
	uint8_t MSB_temp, LSB_temp;
	u16ToMSBandLSB(TopicLen, &MSB_temp, &LSB_temp);
	TName->MSB_Length = MSB_temp;
	TName->LSB_Length = LSB_temp;
	TName->Topic = Topic;
}


/* Function return length of applied bytes.
 */
uint16_t BuildConnectMessage(uint8_t *Message, int MessageLen, uint16_t KeepAlive){
	FixedHeader FHeader;
	ConnectVariableHeader CVHeader;
	uint8_t MSB_temp, LSB_temp, flags;
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
	tempLen += sizeof(FHeader) + 2 + strlen((char*)Topic) + 2 +strlen((char*)Message);
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
	tempLen = strlen((char*)Message);
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
	return ACKSimpleFunctionTemplate(PUBACK, Buffer, BufferLen, packetID);
}

uint16_t BuildPubRecMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	return ACKSimpleFunctionTemplate(PUBREC, Buffer, BufferLen, packetID);
}

uint16_t BuildPubRelMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	return ACKSimpleFunctionTemplate(PUBREL, Buffer, BufferLen, packetID);
}

uint16_t BuildPubCompMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	return ACKSimpleFunctionTemplate(PUBCOMP, Buffer, BufferLen, packetID);
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
		tempLen += 2 + strlen((char*)Topic[i]) + 1;
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
		tempLen = strlen((char*)(Topic[i]));
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


uint16_t BuildUnSubscribeMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topic, uint8_t TopicNo, uint16_t *packetID){
	FixedHeader FHeader;
	UnSubscribeVariableHeader USVHeader;
	uint8_t MSB_temp, LSB_temp, i = 0;
	int tempLen = 0, index = 0;

	Init_FixedHeader(&FHeader, UNSUBSCRIBE, 0);

	srand ( time(NULL) );
	*packetID = (uint16_t) (rand() % 65535);

	//Check if all data can be stored in Message buffer
	tempLen += sizeof(FHeader) + sizeof(USVHeader);
	for (i=0; i<TopicNo; i++)
		tempLen += 2 + strlen((char*)Topic[i]);
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	u16ToMSBandLSB(*packetID, &MSB_temp, &LSB_temp);
	USVHeader.MSB_PacketIdentifier = MSB_temp;
	USVHeader.LSB_PacketIdentifier = LSB_temp;

	index = sizeof(FHeader);
	memcpy(Buffer+index, &USVHeader, sizeof(USVHeader));
	index += sizeof(USVHeader);

	//**** Payload part *****//
	for (i=0; i<TopicNo; i++){
		tempLen = strlen((char*)Topic[i]);
		u16ToMSBandLSB(tempLen, &MSB_temp, &LSB_temp);
		Buffer[index] = MSB_temp;
		Buffer[index+1] = LSB_temp;
		index += 2;
		memcpy(Buffer + index, Topic[i], tempLen);
		index += tempLen;
	}
	//**** Payload part *****//

	u16ToMSBandLSB(index-sizeof(FixedHeader), &MSB_temp, &LSB_temp);
	FHeader.RemainingLengthMSB = MSB_temp;
	FHeader.RemainingLengthLSB = LSB_temp;
	memcpy(Buffer, &FHeader, sizeof(FHeader));

	return (uint16_t) index;
}


uint16_t BuildUnSubACKMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID){
	return ACKSimpleFunctionTemplate(UNSUBACK, Buffer, BufferLen, packetID);
}

/*
 * Function used as a template for such packets like PubACK, PubRel, PubRec, PubComp, UnSubscribeACK.
 * Make packets containing only fixed header and variable header (packet ID).
 */
uint16_t ACKSimpleFunctionTemplate(uint8_t MessageType, uint8_t *Buffer, int BufferLen, uint16_t packetID){
	FixedHeader FHeader;
	DummyStruct DummyVHeader;
	uint8_t MSB_temp, LSB_temp;
	int tempLen = 0, index = 0;

	//Check if all data can be stored in Message buffer
	tempLen = sizeof(FHeader) + sizeof(DummyVHeader);
	if (tempLen > BufferLen)
		return 0;
	tempLen = 0;
	//**************************************************

	Init_FixedHeader(&FHeader, MessageType, 0);
	FHeader.RemainingLengthMSB = 0;
	FHeader.RemainingLengthLSB = 2;

	u16ToMSBandLSB(packetID, &MSB_temp, &LSB_temp);
	DummyVHeader.MSB_PacketIdentifier = MSB_temp;
	DummyVHeader.LSB_PacketIdentifier = LSB_temp;

	memcpy(Buffer, &FHeader, sizeof(FHeader));
	index = sizeof(FHeader);
	memcpy(Buffer+index, &DummyVHeader, sizeof(DummyVHeader));
	index += sizeof(DummyVHeader);

	return (uint16_t) index;
}

uint16_t BuildPingReq(uint8_t *Buffer, int BufferLen){
	return VerySimpleBuildingTemplate(PINGREQ, Buffer, BufferLen);
}

uint16_t BuildPingResp(uint8_t *Buffer, int BufferLen){
	return VerySimpleBuildingTemplate(PINGRESP, Buffer, BufferLen);
}

uint16_t BuildDisconnect(uint8_t *Buffer, int BufferLen){
	return VerySimpleBuildingTemplate(DISCONNECT, Buffer, BufferLen);
}

/*
 * Function used as a template for such packets like PingReq, PingResp, Disconnect.
 * Make packets containing only fixed header.
 */
uint16_t VerySimpleBuildingTemplate(uint8_t MessageType, uint8_t *Buffer, int BufferLen){
	FixedHeader FHeader;

	//Check if all data can be stored in Message buffer
	if (sizeof(FHeader) > BufferLen)
		return 0;
	//**************************************************

	Init_FixedHeader(&FHeader, MessageType, 0);
	FHeader.RemainingLengthMSB = 0;
	FHeader.RemainingLengthLSB = 0;
	memcpy(Buffer, &FHeader, sizeof(FHeader));

	return (uint16_t) sizeof(FHeader);
}

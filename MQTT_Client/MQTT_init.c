/*
 * MQTT.c
 *
 *  Created on: Jan 16, 2016
 *      Author: rudy
 */
#include "MQTT.h"
#include <string.h>


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
	uint16_t MSBtemp, LSBtemp;
	Init_ProtocolName(&CVHeader->PName);
	CVHeader->ProtocolLevel = 4;
	CVHeader->ConnectFlags = Flags;

	u16ToMSBandLSB(KeepAlive, &MSBtemp, &LSBtemp);
	CVHeader->LSB_KeepAlive = LSBtemp;
	CVHeader->MSB_KeepAlive = MSBtemp;
}

void Init_TopicName(TopicName *TName, uint8_t *Topic){
	uint16_t TopicLen = strlen(Topic);
	uint8_t MSB_temp, LSB_temp;
	u16ToMSBandLSB(TopicLen, &MSB_temp, &LSB_temp);
	TName->MSB_Length = MSB_temp;
	TName->LSB_Length = LSB_temp;
	TName->Topic = Topic;
}

void u16ToMSBandLSB(uint16_t u16bit, uint8_t *MSB, uint8_t *LSB){
	uint16_t temp;
	*LSB = (uint8_t) u16bit;
	temp = u16bit>>8;
	*MSB = (uint8_t) temp;
}

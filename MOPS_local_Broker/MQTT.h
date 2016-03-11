/**
 *	@brief Header file for MQTT protocol basic functionality.
 *
 *	Implementation for set of MQTT basic functionalities. Most of them
 *	is responsible for preparing MQTT headers, structures and payloads
 *	(MQTT messages) in correct form.
 *
 *	@file	MQTT.h
 *	@date	Jan 30, 2016
 *	@author	Michal Oleszczyk
 */

#ifndef MQTT_H_
#define MQTT_H_

#include <stdint.h>
#include "MQTTConf.h"
//#include "MOPS.h"


typedef struct FixedHeader{
	uint8_t Flags : 4;
	uint8_t MessageType : 4;
	uint8_t RemainingLengthMSB;
	uint8_t RemainingLengthLSB;
}FixedHeader;

typedef struct DummyStruct{
	uint8_t MSB_PacketIdentifier;
	uint8_t LSB_PacketIdentifier;
}DummyStruct;


// **** Connection structures **** //
typedef struct ProtocolName{
	uint8_t MSB_Length; // = 0
	uint8_t LSB_Length; // = 4
	uint8_t Name[4]; 	// = {'M', 'Q', 'T', 'T'};
}ProtocolName;

typedef struct ConnectVariableHeader{
	ProtocolName PName;
	uint8_t ProtocolLevel; // = 4
	uint8_t ConnectFlags;
	uint8_t MSB_KeepAlive;
	uint8_t LSB_KeepAlive;
}ConnectVariableHeader;

typedef struct ConnACKVariableHeader{
	uint8_t ConnectACKFlags;
	uint8_t ConnectReturnCode;
}ConnACKVariableHeader;
// **** Connection structures **** //



// ***** Publish structures ***** //
typedef struct TopicName{
	uint8_t MSB_Length;
	uint8_t LSB_Length;
	uint8_t *Topic;
}TopicName;

typedef struct PublishVariableHeader{
	TopicName PName;
	uint8_t MSB_PacketIdentifier;
	uint8_t LSB_PacketIdentifier;
}PublishVariableHeader;

typedef DummyStruct PubACKVariableHeader;
typedef DummyStruct PubCompVariableHeader;
typedef DummyStruct PubRelVariableHeader;
typedef DummyStruct PubRecVariableHeader;
// ***** Publish structures ***** //


// **** Subscribe structures **** //
typedef DummyStruct SubscribeVariableHeader;
typedef DummyStruct UnSubscribeVariableHeader;
// **** Subscribe structures **** //

/**
 * @enum MESSAGE_TYPE
 * @brief Enumerator of available MQTT message types.
 */
enum MESSAGE_TYPE{
	CONNECT = 1,	//!< CONNECT message type
	CONNACK,		   //!< CONNACK message type
	PUBLISH,		   //!< PUBLISH message type
	PUBACK,			   //!< PUBACK message type
	PUBREC,			   //!< PUBREC message type
	PUBREL,			   //!< PUBREL message type
	PUBCOMP,		   //!< PUBCOMP message type
	SUBSCRIBE,		 //!< SUBSCRIBE message type
	SUBACK,			   //!< SUBACK message type
	UNSUBSCRIBE,	//!< UNSUBSCRIBE message type
	UNSUBACK,		  //!< UNSUBACK message type
	PINGREQ,     //!< PINGREQ message type
	PINGRESP,    //!< PINGRESP message type
	DISCONNECT,  //!< DISCONNECT message type
};

/**
 * @enum CONNACK_RETURN_CODE
 * @brief List of available MQTT return codes.
 */
enum CONNACK_RETURN_CODE{
	CONNECTION_ACCEPTED = 0,  //!< CONNECTION_ACCEPTED return code
	UNACCEPTABLE_PROTOCOL_VER,//!< UNACCEPTABLE_PROTOCOL_VER return code
	IDENTIFIER_REJECTED,      //!< IDENTIFIER_REJECTED return code
	SERVER_UNAVAILABLE,       //!< SERVER_UNAVAILABLE return code
	BAD_NAME_OR_PASSWORD,     //!< BAD_NAME_OR_PASSWORD return code
	NOT_AUTHORIZED,           //!< NOT_AUTHORIZED return code
};

//Definitions for general purposes
void Init_FixedHeader(FixedHeader *FHeader, uint8_t MessageType, uint8_t Flags);
uint16_t ACKSimpleFunctionTemplate(uint8_t MessageType, uint8_t *Buffer, int BufferLen, uint16_t packetID);
uint16_t VerySimpleBuildingTemplate(uint8_t MessageType, uint8_t *Buffer, int BufferLen);

//Definitions for CONNECT packet
void Init_ProtocolName(ProtocolName *PName);
void Init_ConnectVariableHeader(ConnectVariableHeader *CVHeader, uint8_t Flags, uint16_t KeepAlive);
uint16_t BuildConnectMessage(uint8_t *Message, int MessageLen, uint16_t KeepAlive);
uint16_t BuildConnACKMessage(uint8_t *Buffer, int BufferLen, uint8_t SessionPresent, uint8_t ReturnCode);


//Definitions for PUBLISH packet
void Init_TopicName(TopicName *TName, uint8_t *Topic);
uint16_t BuildClientPublishMessage(uint8_t *Buffer, int BufferLen, uint8_t* Topic, uint8_t* Message, uint8_t DUP, uint8_t Retain, uint16_t *packetID);
uint16_t BuildPubACKMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID);
uint16_t BuildPubRecMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID);
uint16_t BuildPubRelMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID);
uint16_t BuildPubCompMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID);

//Definitions for SUBSCRIBE packet
uint16_t BuildSubscribeMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topic, uint8_t *QoS, uint8_t TopicNo, uint16_t *packetID);
uint16_t BuildSubACKMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID, uint8_t *QoSReturnCode, uint8_t TopicNo);
uint16_t BuildUnSubACKMessage(uint8_t *Buffer, int BufferLen, uint16_t packetID);
uint16_t BuildUnSubscribeMessage(uint8_t *Buffer, int BufferLen, uint8_t **Topic, uint8_t TopicNo, uint16_t *packetID);

//Definitions for other packets
uint16_t BuildPingReq(uint8_t *Buffer, int BufferLen);
uint16_t BuildPingResp(uint8_t *Buffer, int BufferLen);
uint16_t BuildDisconnect(uint8_t *Buffer, int BufferLen);

#endif /* MQTT_H_ */

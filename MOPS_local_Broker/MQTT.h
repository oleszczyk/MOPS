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



/**
 * @struct FixedHeader
 * @brief Structure containing fields to build
 * standard 'fixed header' for MQTT.
 *
 * */
typedef struct FixedHeader{
	/** 4 bits of flags (1-retain, 2-QoS, 1-duplicate).*/
	uint8_t Flags : 4;
	/** MQTT message type. */
	uint8_t MessageType : 4;
	/** Message length (without fixed header) most significant byte. */
	uint8_t RemainingLengthMSB;
	/** Message length (without fixed header) least significant byte. */
	uint8_t RemainingLengthLSB;
}FixedHeader;

/**
 * @struct DummyStruct
 * @brief Structure used for type casting into
 * specific 2-byte 'variable header' in MQTT.
 * */
typedef struct DummyStruct{
	/** Packet identifier most significant byte. */
	uint8_t MSB_PacketIdentifier;
	/** Packet identifier least significant byte. */
	uint8_t LSB_PacketIdentifier;
}DummyStruct;


// **** Connection structures **** //
/**
 * @struct ProtocolName
 * @brief Structures defines fields for
 * 'protocol name' header in MQTT.
 * */
typedef struct ProtocolName{
	/** Protocol name most significant byte. */
	uint8_t MSB_Length; // = 0
	/** Protocol name  least significant byte. */
	uint8_t LSB_Length; // = 4
	/** Protocol name in string form. */
	uint8_t Name[4]; 	// = {'M', 'Q', 'T', 'T'};
}ProtocolName;

/**
 * @struct ConnectVariableHeader
 * @brief Defines specific 'variable header'
 * used in 'Connection' message type in MQTT.
 * */
typedef struct ConnectVariableHeader{
	/** Protocol name structure. */
	ProtocolName PName;
	/** Protocol level value. */
	uint8_t ProtocolLevel; // = 4
	/** Flags defines connection options. */
	uint8_t ConnectFlags;
	/** Keep Alive most significant byte. */
	uint8_t MSB_KeepAlive;
	/** Keep Alive least significant byte. */
	uint8_t LSB_KeepAlive;
}ConnectVariableHeader;

/**
 * @struct ConnACKVariableHeader
 * @brief Defines specific 'variable header'
 * used in 'Connection Acknowledgment' message type in MQTT.
 * */
typedef struct ConnACKVariableHeader{
	/** Flags defines connection acknowledgment options. */
	uint8_t ConnectACKFlags;
	/** Connection acknowledgment return code. */
	uint8_t ConnectReturnCode;
}ConnACKVariableHeader;
// **** Connection structures **** //



// ***** Publish structures ***** //
/**
 * @struct TopicName
 * @brief Structure contains fields
 * to create 'Topic Name' object needed
 * for publish MQTT message.
 * */
typedef struct TopicName{
	/** Topic length most significant byte. */
	uint8_t MSB_Length;
	/** Topic length least significant byte. */
	uint8_t LSB_Length;
	/** Pointer to specified topic.*/
	uint8_t *Topic;
}TopicName;

/**
 * @struct PublishVariableHeader
 * @brief Defines specific 'variable header'
 * used in 'Publish' message type in MQTT.
 * */
typedef struct PublishVariableHeader{
	/** 'Topic Name' structure containing topic length and pointer to it.*/
	TopicName PName;
	/** Packet identifier most significant byte. */
	uint8_t MSB_PacketIdentifier;
	/** Packet identifier least significant byte. */
	uint8_t LSB_PacketIdentifier;
}PublishVariableHeader;

/**
 * @typedef PubACKVariableHeader
 * @brief Definition of 'variable header' for
 * 'publish acknowledgment' message type in MQTT.
 */
typedef DummyStruct PubACKVariableHeader;
/**
 * @typedef PubCompVariableHeader
 * @brief Definition of 'variable header' for
 * 'publish complete' message type in MQTT.
 */
typedef DummyStruct PubCompVariableHeader;
/**
 * @typedef PubRelVariableHeader
 * @brief Definition of 'variable header' for
 * 'publish release' message type in MQTT.
 */
typedef DummyStruct PubRelVariableHeader;
/**
 * @typedef PubRecVariableHeader
 * @brief Definition of 'variable header' for
 * 'publish receive' message type in MQTT.
 */
typedef DummyStruct PubRecVariableHeader;
// ***** Publish structures ***** //


// **** Subscribe structures **** //
/**
 * @typedef SubscribeVariableHeader
 * @brief Definition of 'variable header' for
 * 'subscribe' message type in MQTT.
 */
typedef DummyStruct SubscribeVariableHeader;
/**
 * @typedef SubscribeVariableHeader
 * @brief Definition of 'variable header' for
 * 'unsubscribe' message type in MQTT.
 */
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

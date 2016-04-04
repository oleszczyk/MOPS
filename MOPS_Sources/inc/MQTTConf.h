/**
 *	@brief Configuration file for MQTT protocol basic functionality.
 *
 *	Containing basic definitions which can help user in
 *	standard MQTT configuration.
 *
 *	@file	MQTTConf.h
 *	@date	Jan 30, 2016
 *	@author	Michal Oleszczyk
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/*******************************
*     Connection settings      *
*                              *
*******************************/
//Connection flags settings:
#define CLEANSESSION 0  // 0 - 1
#define WILLFLAG     0  // 0 - 1
#define WILLQOS		 0  // 0 - 2
#define WILLRETAIN   0  // 0 - 1
#define PASSWORDFLAG 0  // 0 - 1
#define USERNAMEFLAG 0  // 0 - 1

#define CLIENTID 		"MojRobot"

//*** Connection flag payload ***//
#if WILLFLAG == 1
	#define WILLTOPIC   "Topic"
	#define WILLMESSAGE "Message"
#endif

#if USERNAMEFLAG == 1
	#define USERNAME 	"username"
#else
	#define PASSWORDFLAG 0
#endif


#if PASSWORDFLAG == 1
	#define PASSWORD 	"password"
#endif
//*******************************//


/*******************************
*  Client Publishing settings  *
*                              *
*******************************/
#define QOS  0      // 0 - 2


#endif /* CONFIG_H_ */

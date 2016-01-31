#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>



#include "MQTTConf.h"
#include "MQTT.h"


#define BUFLEN 44
#define Len2 2

void receive_data(int socket, struct sockaddr_in *si_other);
int connect_to_socket(__const uint8_t *ip_addr, uint16_t port_number, struct sockaddr_in *si_other);
void send_data(uint8_t *buffer, uint16_t buffer_len, int socket, struct sockaddr_in *si_other);
void close_socket(int socket);

int main(void)
{
	struct sockaddr_in si_other;
	int socket;
	uint16_t len;
	uint16_t packetID;
	uint8_t Message[255];

	uint8_t *topic[2]= {"Wysylam", "Cos"};
	uint8_t qos[][1] = {{2}, {2}, {1}};

	//len = BuildConnectMessage(Message, 23, 20);
	//len = BuildSubscribeMessage(Message, 21, topic, qos, 2, &packetID);
	len = BuildDisconnect(Message, 255);

	//printf("\n");
	printf("%d, %d \n", len, packetID);

	socket = connect_to_socket(BROKER_IP, PORT_NUM, &si_other);
	send_data(Message,len, socket, &si_other);
	receive_data(socket, &si_other);
	//close_socket(socket);

	return 0;
}

void receive_data(int socket, struct sockaddr_in *si_other){
	char buf[20];
	int written = 0;
	int slen=sizeof(*si_other);
	written = recvfrom(socket, buf, 20, 0, si_other, &slen);
	printf("written: %d \n", written);
	printf("Przyszlo z %s:%d\nData: %c\n\n",
					inet_ntoa(si_other->sin_addr), ntohs(si_other->sin_port), buf[8]);
}

int connect_to_socket(__const uint8_t *ip_addr, uint16_t port_number, struct sockaddr_in *si_other){

	int s, slen=sizeof(*si_other);
	int enable = 1;
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		perror("socket");
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));

	memset((char *) si_other, 0, sizeof(*si_other));
	si_other->sin_family = AF_INET;
	si_other->sin_addr.s_addr= htonl(INADDR_BROADCAST);
	si_other->sin_port = htons(port_number);


	bind(s,(struct sockaddr *) si_other,sizeof(*si_other));

	//if (inet_aton(INADDR_BROADCAST, &si_other->sin_addr)==0) {
	//	perror("inet_atom()");
	//}
	return s;
}

void send_data(uint8_t *buffer, uint16_t buffer_len, int socket, struct sockaddr_in *si_other){

	if(sendto(socket, buffer, buffer_len, 0, si_other, sizeof(*si_other))==-1){
		perror("Sendto");
		printf("cos poszlo nie tak \n");
	}
	//else
	//	printf("Wyslano \n");
}

void close_socket(int socket){
	close(socket);
}

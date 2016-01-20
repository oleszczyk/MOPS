#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#include "MOPS.h"


int main(void)
{
	fd_set read_fd;
	struct timeval tv;
    int s, len, rv;
    uint8_t topic[][1]={"Topic"};
    uint8_t Qos[][1]={1};
    uint8_t buffer[100];


	s = connectMOPS();
	subscribeMOPS(topic, Qos);

	FD_ZERO(&read_fd);
	FD_SET(s, &read_fd);

	for(;;){
	    tv.tv_sec = 0;
	    tv.tv_usec = 500000;

		publishMOPS("Cos2", "Message", 4);
		rv = select(s+1, &read_fd, NULL, NULL, &tv);
		if(rv > 0){
			len = readMOPS(s, buffer, 100);

			printf("%s \n", buffer);

		}
		if(rv < 0 ){
			perror("selecet");
		}
		if( rv == 0 ){
			perror("selecet");

		}

	}
    close(s);
    return 0;
}

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
    int s;
    int i;
    uint8_t array[100];
    uint8_t *topic[]={"TopicTopicTopic", "Gowno", "niktktosnikt"};
    uint8_t Qos[]={1, 2, 1};

	s = connectMOPS();
	subscribeMOPS(topic, Qos, 3);
	for(;;){
	    usleep(100000);
		publishMOPS(s, "Gowno", "cos tam sle");
		i = readMOPS(array, 100);

		printf("dlugosc: %d, %s\n",i, array);

	}
    //close(s);
    return 0;
}

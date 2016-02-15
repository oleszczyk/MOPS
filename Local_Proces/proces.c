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
    uint8_t topic[][1]={"Topic"};
    uint8_t Qos[][1]={1};

	s = connectMOPS();
	subscribeMOPS(topic, Qos);

	for(;;){
	    sleep(1);
		publishMOPS(s, "nikt", "Bleble");
	}
    //close(s);
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#include "MOPS.h"

#define SEN 1
#define REC 2
#define TYPE SEN   //1 - sender, 2 - receiver



int main(void)
{
    int s;
    uint8_t array[100];
    char *topic[2]={"jakisTopic", "kupa"};
    uint8_t Qos[]={1, 2};

	s = connectMOPS();
#if TYPE == REC
	subscribeMOPS(topic, Qos, 2);
#endif
	for(;;){

#if TYPE == SEN
		usleep(200000);
		publishMOPS(s, (uint8_t*)"jakisTopic", (uint8_t*)"Pierwsza wiadomosc");
#endif
#if TYPE == REC
		i = readMOPS(array, 100);
		printf("dlugosc: %d, %s\n",i, array);
#endif
	}
    //close(s);
    return 0;
}

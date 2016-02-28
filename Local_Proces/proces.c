#include <stdio.h>
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
    int i;
    uint8_t array[100];
    uint8_t *topic[]={"jakisTopic", "kupa"};
    uint8_t Qos[]={1, 2};

	s = connectMOPS();
#if TYPE == REC
	subscribeMOPS(topic, Qos, 2);
#endif
	for(;;){

#if TYPE == SEN
		usleep(200000);
		publishMOPS(s, "jakisTopic", "Pierwsza wiadomosx");
#endif
#if TYPE == REC
		i = readMOPS(array, 100);
		printf("dlugosc: %d, %s\n",i, array);
#endif
	}
    //close(s);
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "MOPS.h"

#define SEN 1
#define REC 2
#define TYPE REC   //1 - sender, 2 - receiver


#if TYPE == REC
int main(void)
{
    int s;
    char array[100];
    char *topic[2]={"jakisTopic", "kupa"};
    uint8_t Qos[]={1, 2};

	s = connectToMOPS();
	subscribeMOPS(topic, Qos, 2);
	for(;;){
		readMOPS(array, 100);
		printf("%s\n", array);
	}
    return 0;
}
#endif //REC



#if TYPE == SEN
int main(void)
{
    char array[100];
    uint8_t Qos[]={1, 2};

	connectToMOPS();
	for(;;){
		usleep(100000);
		publishMOPS("jakisTopic", "Pierwsza wiadomosc");
	}
    return 0;
}
#endif //SEN

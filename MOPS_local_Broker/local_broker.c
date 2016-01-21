/*
 * local_broker.c
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#define SOCK_PATH "./../MOPS_path"


void AddClientIDToPacket(uint8_t *buf, uint8_t ClientID, int *WrittenBytes, int nbytes);

int main(void)
{
	struct timeval tv;
    int t, len, i, rv, nbytes;
    struct sockaddr_un local, remote;
    uint8_t input_buffer[512], output_buffer[512];
    int inputWrittenIndex = 0, outpuWrittenIndex = 0;
    int free_space;

    int licznik = 0;


    fd_set master;  //master fd list
    fd_set read_fd; //temp fd list for select()
	int fdmax;		//maximum fd number

	int listener;   //listening socket descriptor
	int newfd;		//newly accept()ed socket descriptor


	memset(input_buffer, 0, sizeof(input_buffer));
	memset(output_buffer, 0, sizeof(output_buffer));

	FD_ZERO(&master);
	FD_ZERO(&read_fd);

    if ((listener = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(listener, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(listener, 5) == -1) {
        perror("listen");
        exit(1);
    }

    //add listener to the master set
    FD_SET(listener, &master);
    fdmax = listener;


    for(;;){
    	free_space = sizeof(input_buffer)-(inputWrittenIndex*sizeof(input_buffer[0]));
    	tv.tv_sec = 0;
        tv.tv_usec = 10000;
    	read_fd = master;

    	if( free_space <= sizeof(input_buffer)/10 )
    		rv = 0;		//go to "idle state"
		else{
			if((rv = select(fdmax+1, &read_fd, NULL, NULL, &tv)) == -1){
				perror("select");
				exit(4);
			}
		}

    	if(rv > 0){
			for(i = 0; i <=fdmax; i++){
				if (FD_ISSET(i, &read_fd)){
					if(i == listener){
						t = sizeof(remote);
						newfd = accept(listener,(struct sockaddr *)&remote, &t);
						if(newfd == -1){
							perror("accept");
						}
						else{
							FD_SET(newfd, &master);
							if(newfd > fdmax)
								fdmax = newfd;
							printf("Nowy deskryptor: %d \n", newfd);

						}
					}
					else{
						//here we get data from sub_processes or from RTnet
						printf("Space: %d \n", free_space);
						//add new packet to the end of all data
						nbytes = recv(i, input_buffer+inputWrittenIndex, free_space, 0);

						if ( nbytes <= 0 ){
							close(i);
							FD_CLR(i, &master);
						}
						else{
							if (free_space >= ( nbytes + sizeof( (uint8_t) i )) ){
								AddClientIDToPacket(input_buffer+inputWrittenIndex, (uint8_t) i,  &inputWrittenIndex, nbytes);
								inputWrittenIndex += nbytes;
								licznik += 1 ;
							}
							//we need to erase memory which is too small to send data (the last packet)
							else{
								memset(input_buffer+inputWrittenIndex, 0, free_space);
								//better never be here!
							}
						}
					}

				}
			}
    	}
    	if(rv < 0){
    	    perror("select"); // error occurred in select()
    	}
    	if(rv == 0){
            //here make other stuff
    		//reorganize data,
    		//pack everything together and wait for time slot to send everything to RTnet
    		//
    		// unpack data from RTnet
    		// resend data to subprocesses
    		if(licznik > 10){
    			printf("Dane: %s \n", input_buffer);
    			memset(input_buffer, 0, sizeof(input_buffer));
    			inputWrittenIndex = 0;
    			licznik = 0 ;
    		}
    	}

    }
    return 0;
}


void AddClientIDToPacket(uint8_t *buf, uint8_t ClientID, int *WrittenBytes, int nbytes){
	memmove(buf + sizeof(ClientID), buf, nbytes);
	memcpy(buf, &ClientID, sizeof(ClientID));
	(*WrittenBytes) += sizeof(ClientID);
}


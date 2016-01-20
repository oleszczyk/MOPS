/*
 * local_broker.c
 *
 *  Created on: Jan 20, 2016
 *      Author: rudy
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#define SOCK_PATH "./../MOPS_path"

int main(void)
{
	struct timeval tv;
    int t, len, i, rv, nbytes;
    struct sockaddr_un local, remote;
    char str[100];
    fd_set master;  //master fd list
    fd_set read_fd; //temp fd list for select()
	int fdmax;		//maximum fd number

	int listener;   //listening socket descriptor
	int newfd;		//newly accept()ed socket descriptor


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
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
    	read_fd = master;

    	if((rv = select(fdmax+1, &read_fd, NULL, NULL, &tv)) == -1){
    		perror("select");
    		exit(4);
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

						}
					}
					else{
						//here we get data from sub_processes
						nbytes = recv(i, str, 100, 0);
						if ( nbytes <= 0 ){
							close(i);
							FD_CLR(i, &master);
						}
						else{
							if (send(i, str, nbytes, 0) < 0) {
				        		perror("send");
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
    	}

    }
    return 0;
}

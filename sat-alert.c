/****************************************************************************
*                                                                           *
*  This program was originally written by Ivan Galysh, KD4HBO, 24-Jan-2000  *
*  and has been modified extensively since.  Most recently, Bent Bagger,    *
*  OZ6BL, provided a patch to detect the absence of a PREDICT server,       *
*  and print an error message to that effect.                               *
*                                                                           *
*  Prepared for Terrassa Ground Station by Boyan Naydenov, EA3HXM, and 		*
*  Antoni Giménez.															*
*                                                           				*
*****************************************************************************
*                                                                           *
* This program is free software; you can redistribute it and/or modify it   *
* under the terms of the GNU General Public License as published by the     *
* Free Software Foundation; either version 2 of the License or any later    *
* version.                                                                  *
*                                                                           *
* This program is distributed in the hope that it will useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License     *
* for more details.                                                         *
*                                                                           *
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>

char string[625];

void handler()
{
	/* This is a function that is called when the response function
	   times out.  This is in case the server fails to respond. */

	signal(SIGALRM,handler);
}

int connectsock(char *host, char *service, char *protocol)
{
	/* This function is used to connect to the server.  "host" is the
	   name of the computer on which PREDICT is running in server mode.
	   "service" is the name of the socket port.  "protocol" is the
	   socket protocol.  It should be set to UDP. */

	struct hostent *phe;
	struct servent *pse;
	struct protoent *ppe;
	struct sockaddr_in sin;
	
	int s, type;
	
	bzero((char *)&sin,sizeof(struct sockaddr_in));
	sin.sin_family=AF_INET;
	
	if ((pse=getservbyname(service,protocol)))
		sin.sin_port=pse->s_port;

	else if ((sin.sin_port=htons((unsigned short)atoi(service)))==0)
	{
		printf("Can't get services\n");
		return -1;
	}

	if ((phe=gethostbyname(host)))
		bcopy(phe->h_addr,(char *)&sin.sin_addr,phe->h_length);

	else if ((sin.sin_addr.s_addr=inet_addr(host))==INADDR_NONE)
	{
		printf("Can't get host: \"%s\".\n",host);
		return -1;
	}
	
	if ((ppe=getprotobyname(protocol))==0)
		return -1;

	if (strcmp(protocol,"udp")==0)
		type=SOCK_DGRAM;
	else
		type=SOCK_STREAM;
	
	s=socket(PF_INET,type,ppe->p_proto);

	if (s<0)
	{
		printf("Can't get socket.\n");
		return -1;
	}

	if (connect(s,(struct sockaddr *)&sin,sizeof(sin))<0)
	{
		printf("Can't connect to socket.\n");
		return -1;
	}

	return s;
}

void get_response(int sock, char *buf)
{
	/* This function gets a response from the
	   server in the form of a character string. */

	int n;

	n=read(sock,buf,625);

	if (n<0)
	{
		if (errno==EINTR)
			return;

		if (errno==ECONNREFUSED)
		{
			fprintf(stderr, "Connection refused - PREDICT server not running\n");
			exit (1);
		}
	}

	buf[n]='\0';
}

char *send_command(host, command)
char *host, *command;
{
	int sk;

	/* This function sends "command" to PREDICT running on
	   machine "host", and returns the result of the command
	   as a pointer to a character string. */

	/* Open a network socket */
	sk=connectsock(host,"predict","udp");

	if (sk<0)
		exit(-1);

	/* Build a command buffer */
	sprintf(string,"%s\n",command);

	/* Send the command to the server */
   	write(sk,command,strlen(string));

	/* clear string[] so it can be re-used for the response */
	string[0]=0;

	/* Get the response */
   	get_response(sk,string);

	/* Close the connection */
   	close(sk);

	return string;
}

int AddToArray (DATA item)
{
        if(num_elements == num_allocated) // Are more refs required?
        { 
                // Feel free to change the initial number of refs
                // and the rate at which refs are allocated.
                if (num_allocated == 0)
                        num_allocated = 3; // Start off with 3 refs
                else
                        num_allocated *= 2; // Double the number 
                                                    // of refs allocated

                // Make the reallocation transactional 
                // by using a temporary variable first
                void *_tmp = realloc(the_array, (num_allocated * sizeof(DATA)));

                // If the reallocation didn't go so well,
                // inform the user and bail out
                if (!_tmp)
                {
                        fprintf(stderr, "ERROR: Couldn't realloc memory!\n");
                        return(-1);
                }

                // Things are looking good so far
                the_array = (DATA*)_tmp;
        }

        the_array[num_elements] = item;
        num_elements++;

        return num_elements;
}


void send_command2(host, command)
char *host, *command;
{
	int sk;
	int i;

	typedef struct {
    	char *name;
    	int number;
	} DATA;

	DATA    *the_array = NULL;
	int     num_elements = 0; // Keeps track of the number of elements used
	int     num_allocated = 0; // This is essentially how large the array is

	/* This function sends "command" to PREDICT running on
	   machine "host", and displays the result of the command
	   on the screen as text.  It reads the data sent back
	   from PREDICT until an EOF marker (^Z) is received. */

	/* Open a network socket */
	sk=connectsock(host,"predict","udp");

	if (sk<0)
		exit(-1);

	/* Build a command buffer */
	sprintf(string,"%s\n",command);

	/* Send the command to the server */
   	write(sk,command,strlen(string));

	/* clear string[] so it can be re-used for the response */
	string[0]=0;

	/* Read and display the response until a ^Z is received */
   	get_response(sk,string);

	while (string[0]!=26)  /* Control Z */
	{
		printf("%s",string);
		string[0]=0;
   		get_response(sk,string);
	}

	printf("\n");

	/* Close the connection */
   	close(sk);
}

int main()
{
	char buf[128], command[128];
	long aostime, orbitnum, start;
	float az, el, slong, slat, footprint, range, altitude,
	      velocity, phase, eclipse_depth, squint;
	time_t t;



	// if start== saturday then poll for the whole weekend and obtain a resume

	sprintf(command,"PREDICT \"ISS\" %ld",start);
	printf("%s\n",command );
	send_command2("localhost",command);
	// strcpy(buf, send_command("localhost",command));
	// printf("%s",buf);
	// check if buf has Min El of say 20 degrees.


	exit(0);

}
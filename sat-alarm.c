/****************************************************************************
*  Title: Satellite Tracking Alert Serivce for Slack integration.			*
*	Easily adaptable to other polling services such as email.				*
*																			*
*  Authors:        Boyan Naydenov, EA3HXM, and  Antoni Giménez.			    *                                                                  *
*  Socket treatment written by Ivan Galysh, KD4HBO, 24-Jan-2000             *
*  and has been modified extensively since.  Most recently, Bent Bagger,    *
*  OZ6BL, provided a patch to detect the absence of a PREDICT server,       *
*  and print an error message to that effect.                               *
*                                                                           *
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
#include <curl/curl.h>
#include <json-c/json.h>

char string[74];
char url_webhook[500];


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
	/* This function gets a response from the server in the form of a character string. */

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
void post_slack(char daily_prediction[], time_t epoch ) {
	CURL *curl;
	CURLcode res;

	char Date[200];
	char buffer[26];
	char title[200];
	struct tm* tm_info;
	tm_info = gmtime(&epoch);
	strftime(buffer, 26, "%d-%m-%Y", tm_info);
	sprintf(Date,"%s ISS passes", buffer);
	sprintf(title, "Date                          Time (UTC)                 El   Az     Ph    Lat    Lon    R      Orbit ");

	/***************
	*    JSON-C    *
	***************/

	//JSON object
	json_object * jObj = json_object_new_object();
	json_object * subObj = json_object_new_object();
	/*Creating a json array*/
	json_object *jarray = json_object_new_array();

	//Create a string JSON element
	json_object *pretext= json_object_new_string(Date);
	json_object *jString = json_object_new_string(daily_prediction);
	//Create a string JSON element
	json_object *color = json_object_new_string("#36a64f");
	//Create a string JSON element
	json_object *date= json_object_new_string(title);

	json_object_object_add(subObj,"pretext",pretext);
	json_object_object_add(subObj,"text", jString);
	json_object_object_add(subObj,"color",color);
	json_object_object_add(subObj,"title",date);

	json_object_array_add(jarray,subObj);

	json_object_object_add(jObj,"attachments", jarray);

	/***************
	*     CURL     *
	***************/

	//Start Curl with the parameter that starts all the modules,only needed once
	curl_global_init(CURL_GLOBAL_ALL);

	//Create an easy handle curl
	curl = curl_easy_init();

	if(curl) {
		//Define Curl propierties:
		curl_easy_setopt(curl, CURLOPT_URL, url_webhook);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jObj));

		//Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		//Check for errors
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		//Cleanup the easy handle curl created
		curl_easy_cleanup(curl);
	}
	//Cleanup the global curl, cleanup all modules
	curl_global_cleanup();
}

char * send_command(char *host, char *command, int * get_day, long int * get_time) {
	int sk;
	int j = 1;

	char predicted_day[2];
	char predicted_time[9];
	/* This function sends "command" to PREDICT running on
	   machine "host", and returns the result of the command
	   as a pointer to a character string. */

	/* Open a network socket */
	sk=connectsock(host,"predict","udp");
	int lines = 0;
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
	/* Single prediction storage */
	static char *single_prediction;
	char last_line[74];

	single_prediction = malloc( 74 * sizeof(char) );
	strcpy(single_prediction,string);

	while (string[0]!=26)  /* Control Z */
	{
		string[0]=0;
		get_response(sk,string);
		// 26 appears when prediction finishes
		if(string[0]==26) {
			break;
		}
		j++;
		single_prediction = realloc( single_prediction, 74*j * sizeof(char) );
		strcat(single_prediction,string);
		strcpy(last_line,string);
	}
	char end[4];
	sprintf(end,"\n");
	strcat(single_prediction,end);

	int Get_day;
	long int Get_time;
	// Obtain last predicted day
	strncpy(predicted_day, last_line+15, 2);
	predicted_day[2] = '\0';
	sscanf(predicted_day, "%d", &Get_day);

	// Obtain last predicted time
	for(int i = 0; i<=9; i++) {
		predicted_time[i] = last_line[i];
	}
	sscanf(predicted_time, "%ld", &Get_time);

	*get_day = Get_day;
	*get_time = Get_time;

	/* Close the connection */
	close(sk);
	/* Return single prediction result */
	return single_prediction;
}

int to_unix_time(char date[9]) {
	struct tm tm;
	time_t epoch;
	if ( strptime(date, "%d/%m/%Y %H:%M:%S", &tm) != NULL ) {
		// timegm gets the time (as if it is in UTC) and converts it into UNIX time
		epoch = timegm(&tm);
		// printf("TIME: %d\n",(int)epoch);
	}
	else
		printf("Error with time conv");

	return epoch;
}

void get_daily_prediction(int time_step, int today, time_t epoch, char sat_name[]) {
	int the_date = (int)epoch;
	char command[128];
	char * daily_prediction;
	char * single_prediction;

	int get_day = today;
	long int get_time = 1;

	daily_prediction = malloc( 74 * sizeof(char) );
	int n_daily = 0;
	int n_single = 0;
	daily_prediction[0] = '\0';
	while ( get_day==today ) {

		sprintf(command,"PREDICT %s %d", sat_name, the_date);
		single_prediction = send_command("localhost",command, &get_day, &get_time);
		//  In the next loops, we assign a single prediction array to the daily one.
		if (get_day == today) {
			n_single = strlen(single_prediction);
			n_daily = n_daily + n_single;
			daily_prediction = realloc( daily_prediction, n_daily * sizeof(char) );
			strcat(daily_prediction,single_prediction);
			the_date = get_time + time_step;
		}
		free(single_prediction);
		// printf("DAY: %d\n",today);
		// printf("GET_DAY: %d\n",get_day);
		// printf("%ld\n",get_time);
		// printf("\n%d\n\n",the_date);

		// sleep(1);
	}
	printf("%s",daily_prediction);
	post_slack(daily_prediction,epoch);
	free(daily_prediction);
}
void print_help(int exval) {
 printf("sat-alarm [-h] -s <satellite-name> [-d <dd/mm/aaaa>] -u <url-to-post> \n\n");

 printf("  -h              print this help and exit\n");
 printf("  -d              get a date. Default: current date.\n");
 exit(exval);
}

int main(int argc, char **argv)
{
	char sat_name [30];
	int sat = -1;

	char url_name[500];
	int url = -1;

	char date[18];
	int set_date = -1;
	int day, the_date;
	time_t epoch;

	int opt;
	if(argc == 1) {
		fprintf(stderr, "This program needs arguments....\n\n");
		print_help(1);
	}
	while((opt = getopt(argc, argv, "hs:d:u:")) != -1) {
		switch(opt) {
			case 'h':
				print_help(0);
				break;
			case 's':
				sat = 0;
				strcpy(sat_name,optarg);
				break;
			case 'd':
				set_date = 0;
				strcpy(date, optarg);
				strcat(date, " 00:01:00");
				epoch = to_unix_time(date);
				struct tm tm = *gmtime(&epoch);
				day = tm.tm_mday ;
				the_date = (int)epoch;
				break;
			case 'u':
				url = 0;
				strcpy(url_name,optarg);
				strcpy(url_webhook, url_name);
				break;
			case ':':
				fprintf(stderr, "sat-alarm: Error - Option `%c' needs a value\n\n", optopt);
				print_help(1);
				break;
			case '?':
				fprintf(stderr, "sat-alarm: Error - No such option: `%c'\n\n", optopt);
				print_help(1);
		}
	}

	// Check mandatory parameters:
	if (sat == -1 | url == -1) {
		printf("-s  and -u are mandatory!\n");
		exit(1);
	}
	int time_step = 60;
	if(set_date == -1) {
		epoch = time(NULL);
		struct tm tm = *gmtime(&epoch);
		day = tm.tm_mday;
		the_date = (int)epoch;
	}
	get_daily_prediction(time_step,day,epoch,sat_name);

	exit(0);
}
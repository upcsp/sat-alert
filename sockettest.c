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

char string[72];


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

int send_command(char *host, char *command, char single_prediction[20][72]) {
	int sk;
	int j = 0;
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
	get_response(sk,string);
	/* Get the response */
	while (string[0]!=26)  /* Control Z */
	{
		string[0]=0;
		get_response(sk,string);
		for(int k = 0; k<625; k++) {
			single_prediction[j][k] = string[k];
		}
		// printf("%s\n", string );
		j++;
		if(string[0]!=26) {
			lines++;
			for(int k = 0; k<625; k++) {
				single_prediction[j][k] = '-';
			}
			lines++;
		}
	}
	/* Close the connection */
	close(sk);
	return lines;
}

int to_unix_time(char date[9]) {
	struct tm tm;
	time_t epoch;
	if ( strptime(date, "%d/%m/%Y %H:%M:%S", &tm) != NULL ) {
		epoch = mktime(&tm);
		// printf("TIME: %d\n",(int)epoch);
	}
	else
		printf("Error with time conv");

	return epoch;
}
void postSlack(char daily_prediction[], time_t epoch ) {
    CURL *curl;
    CURLcode res;

    char Date[200];
    // time_t timer;
    char buffer[26];
    struct tm* tm_info;

    // time(&epoch);
    tm_info = gmtime(&epoch);

    strftime(buffer, 26, "%d-%m-%Y", tm_info);

    sprintf(Date,"%s ISS passes", buffer);



    /***************
    *    JSON-C    *
    ***************/

    //First we create a JSON object
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
    json_object *date= json_object_new_string("Date                          Time                        El   Az  Phase  LatN   LonW    Range  Orbit ");

    //Include the string element jString in json_object as "text":"TEST MESSAGE"
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

        curl_easy_setopt(curl, CURLOPT_URL, "https://hooks.slack.com/services/T2C3W8GGM/B4KSZSFGU/xxxxxxxxx");

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
void saveToFile(char full[]) {
	FILE *f = fopen("daily_prediction.txt", "w");
	if (f == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	fprintf(f,"%s\n", full);
	fclose(f);
}
void get_daily_prediction(int time_step, char *argv[], int mode) {

	int day, the_date;
	time_t epoch;
	if(mode==1) {
		//  format like DD/MM/AAAA
		char date[18];
		strcpy(date, argv[1]);
		strcat(date, " 00:01:00");
		epoch = to_unix_time(date);
		struct tm tm = *gmtime(&epoch);
		day = tm.tm_mday + 1;
		the_date = (int)epoch;
	}
	else {
		epoch = time(NULL);
		struct tm tm = *gmtime(&epoch);
		day = tm.tm_mday + 1;
		the_date = (int)epoch;
	}


	char day_str[1],command[128],daily_prediction[800][72], single_prediction[20][72], last_line[72], line[72];

	int index = 0;
	int i = 0,j = 0,k = 0;

	char predicted_day[2];
	int get_day = day;

	char predicted_time[9];
	long int get_time = 0;

	int islastline;

	int total_lines = 0;
	int lines = 0;
	while ( get_day==day ) {
		last_line[0] = 0;
		islastline = 0;
		sprintf(command,"PREDICT \"ISS\" %d", the_date);
		lines = send_command("localhost",command, single_prediction);
		total_lines = total_lines + lines;
		//  In the next loops, we assign a single prediction array to the daily one. Also, we get the last line of info.
		for (i=0; i<lines+1; i++) {
			for (j=0; j<72; j++) {
				// If we hit the last line, then we will save the previous line which is the one with the info.
				if(single_prediction[i][j]==26) {
					for(k=0; k<72; k++) {
						last_line[k] = single_prediction[i-1][k];
					}
					daily_prediction[index][0] = '\n';
					// printf("LAST LINE %s\n", last_line );
					islastline = 1;
					break;
				}
				if(single_prediction[i][j]=='\0') {
					printf("KAKAKAK");
					break;
				}
				line[j] = single_prediction[i][j];
			}
			// Check if predicted day is still the same as day in order not to save other data
			strncpy(predicted_day, line+15, 2);
			predicted_day[2] = '\0';
			sscanf(predicted_day, "%d", &get_day);
			if (get_day == day & line[0]!='\0') {
				for (j=0; j<72; j++) {
					daily_prediction[index][j] = line[j];
					if(j==71) {
						daily_prediction[index][j] = '\n';
					}
				}
			}
			if(islastline==1){
				index++;
				for (j=0; j<72; j++) {
					daily_prediction[index][j] = '-';
					if(j==71)
						daily_prediction[index][j] = '\n';
				}
				index++;
				total_lines++;
				break;
			}
			index++;
		}
		strncpy(predicted_day, last_line+15, 2);
		predicted_day[2] = '\0';
		// conversion from char to int
		sscanf(predicted_day, "%d", &get_day);

		for(int i = 0; i<=9; i++) {
			predicted_time[i] = last_line[i];
		}
		sscanf(predicted_time, "%ld", &get_time);

		the_date = get_time + time_step;

		// printf("%d\n",day);
		// printf("%d\n",get_day);
		// printf("%ld\n",get_time);
		// printf("\n%d\n\n",the_date);

		// sleep(1);
	}
	char full[total_lines*72];
	int ka=0;
	for (i=2; i<total_lines; i++) {
		for (j=0; j<72; j++) {
				if(daily_prediction[i][j]=='\0'){
				// printf("NULL CHARACTERS EXIST!!!! \n i: %d\n j: %d\n",i,j);
					break;
				}
				else {
					full[ka] = daily_prediction[i][j];
				}
			ka++;
			printf("%c",daily_prediction[i][j]);
		}
	}
	int n = sizeof(full)/sizeof(full[0]);
	full[n] = '\0';
	// printf("%s\n",full);
	// saveToFile(full);
	postSlack(full,epoch);

}



int main(int argc, char **argv)
{
	int mode;
	int time_step = 60;

	if(argc>1) {
		mode = 1;
	}
	else {
		mode = 0;
	}
	get_daily_prediction(time_step, argv, mode);

	exit(0);
}

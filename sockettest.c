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

void send_command2(host, command)
char *host, *command;
{
    int sk;

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


char *get_time(){
    return send_command("localhost","GET_TIME$");
}

void get_daily_prediction(char time_set[128], int time_step) {
    char day_str[1],command[128],prediction[4000]; 
    int int_time = (unsigned long)time(NULL);

    // Get day as int
    day_str[0] = time_set[8];
    day_str[1] = time_set[9];
    int day;


    sscanf(day_str, "%d", &day);
    // I HAVE TO CREATE THIS STUPID VARIABLE BECAUSE FOR SOME REASON, day CHANGES INSIDE THE WHILE LOOP
    //int Tday = day;
    int Tday = 29;
    char predicted_day[1];
    int get_day = 29;
    char predicted_time[9];
    long int get_time = 0;

    // For some reason, this while works. Even if the condition
    while ( get_day==Tday ) {
        sprintf(command,"PREDICT \"ISS\" %ld", int_time);
        strcpy(prediction, send_command("localhost",command));
        send_command2("localhost",command);

        // here we should get the one from the last line!!!! WARNING: we are working just with the first line!! All the lines are shown because of send_command2
        predicted_day[0] = prediction[15];
        predicted_day[1] = prediction[16];
        sscanf(predicted_day, "%d", &get_day);

        for(int i = 0; i<=9; i++) {
            predicted_time[i] = prediction[i];
        }
        sscanf(predicted_time, "%d", &get_time);

        int_time = get_time + time_step;

        // printf("%d\n",int_time);
        // printf("%d\n",get_day);
        // printf("%d\n",Tday);
        //printf("%s\n",prediction);
        sleep(1);
    }
}



int main()
{
    char satlist[625], time_set[128], command[128], prediction[4000];
    // IMPORTANT: we have to put a bigger time that a normal pass, becuase otherwise, if it is smaller we keep quering for the same output!! Repair above function!!
    int time_step = 1000;
    
    /* Get the list of satellite names from PREDICT 
    strcpy(satlist, send_command("localhost","GET_LIST"));
    printf("\nPREDICT returned the following string in response to GET_LIST:\n%s\n",satlist);*/


    strcpy(time_set, get_time());
    printf("The following string was returned in response to GET_TIME$:\n%s\n",time_set);

    get_daily_prediction(time_set, time_step);
    
    exit(0);
}
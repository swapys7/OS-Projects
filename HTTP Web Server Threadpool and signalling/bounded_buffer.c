#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "csuc_http_swapniljoshi.h"
#include "csuc_http.h"
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
int bSIZE ;

void consumer()
{

static sigset_t signal_mask;  /* signals to block         */
static sigset_t old_signal_mask;

int rc;
    sigemptyset (&signal_mask);
    sigaddset (&signal_mask, SIGINT);
    sigaddset (&signal_mask, SIGTERM);
    sigaddset (&signal_mask, SIGUSR1);
    sigaddset (&signal_mask, SIGUSR2);

    rc = pthread_sigmask (SIG_BLOCK, &signal_mask, &old_signal_mask);
    if (rc != 0) 
    {
       log_info(0,"problem here in consumer");
        
    }

int cfd;
log_info(3,"consumer started.\n");


 	while(1){
	
 	pthread_mutex_lock(&(buffer.mutex) );

 	if (buffer.occupied <= 0) log_info(3,"consumer waiting.\n");
 	while(buffer.occupied <= 0)
   	 pthread_cond_wait(&(buffer.more), &(buffer.mutex) );
 	log_info(3,"consumer executing.\n");

 	cfd = buffer.buf[buffer.nextout++];

 	http_request_t *request;
http_response_t *response;
request  = (http_request_t *)malloc(sizeof(http_request_t));   // Memory allocation for request header
response = (http_response_t *)malloc(sizeof(http_response_t)); // Memory allocation for response header
strcpy(request->uri,root_path);
int sock_fd = cfd; 
int next_request_return_value = next_request(sock_fd, request);
int build_ret_val = build_response(request,response);
int send_response_ret_val = send_response(sock_fd,response);
clock_gettime( CLOCK_REALTIME, &stop);
total_time_servicing_request_seconds.tv_sec = total_time_servicing_request_seconds.tv_sec + abs( stop.tv_sec - start.tv_sec );
total_time_servicing_request_seconds.tv_nsec= total_time_servicing_request_seconds.tv_nsec + abs( stop.tv_nsec - start.tv_nsec );
free(request);
free(response);
//free((int*)arg);

 	buffer.nextout %= bSIZE;
 	buffer.occupied--;


 pthread_cond_signal(&(buffer.less));
 pthread_mutex_unlock(&(buffer.mutex));
 }
log_info(3,"consumer exiting.\n");
 //pthread_exit(0);
    free(buffer.buf);
   	pthread_exit(pthread_self);
}

void * producer(void * parm)
{
	log_info(3,"Entered into producer.\n");	 
 int i, cfd;
 int ptr_cfd;
 buffer.buf = malloc(sizeof(int)*bSIZE);
 while(status==RUNNING)
{
	log_info(3,"Entered into while of producer.\n");
 	cfd = accept(lfd, (struct sockaddr *) NULL, NULL);
	clock_gettime( CLOCK_REALTIME, &start); 
	if(cfd==-1)
	{
	  continue;
	}
	no_request_handled = no_request_handled+1;
	pthread_mutex_lock(&(buffer.mutex));

     	if (buffer.occupied >= bSIZE) 
	log_info(3,"producer waiting.\n");

     	while (buffer.occupied >= bSIZE)
	pthread_cond_wait(&(buffer.less), &(buffer.mutex) );
     	log_info(3,"producer executing.\n");

	pthread_t tid;
	ptr_cfd = cfd;
	buffer.buf[buffer.nextin++] = ptr_cfd;
     	buffer.nextin %= bSIZE;
     	buffer.occupied++;

     pthread_cond_signal(&(buffer.more));
     pthread_mutex_unlock(&(buffer.mutex));

      }

log_info(3,"Coming out of the while of producer.\n");
 log_info(3,"producer exiting.\n");
 //pthread_exit(0);
exit(EXIT_SUCCESS);
}

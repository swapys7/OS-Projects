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
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include "status_lookup.h"

#define ERROR 0
#define WARNING 1
#define INFO 2
#define DEBUG 3
#define BILLION  1000000;

volatile sig_atomic_t status = RUNNING;

#define BACKLOG 10
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT" //Define time format in required GMT format
int bSIZE =16;

void graceful_shutdown()
{
    status = SHUTDOWN;
}

void print_Env_and_stat_variables()
{

	clock_gettime( CLOCK_REALTIME, &stop_sec);
	pthread_mutex_lock(&(TOTAL_UP_TIME.mutex3));
	seconds.tv_sec = seconds.tv_sec + abs( stop_sec.tv_sec - start_up.tv_sec );
	seconds.tv_nsec= seconds.tv_nsec + abs( stop_sec.tv_nsec - start.tv_nsec);	
	pthread_mutex_unlock(&(TOTAL_UP_TIME.mutex3));

	if(strategy == "FORK")
	{	
		log_info(4,"\n *** INFORMATION REGARDING ENVIRONMENTAL AND STATIC VARIABLES ***\n");
		log_info(4,"\n The BOUNDED BUFFER SIZE IS          	    : %d", bSIZE);
		log_info(4,"\n The PORT NUMBER IS                  	    : %d", port_number);
		log_info(4,"\n The DOCUMENT ROOT IS                	    : %s", root_path);
		log_info(4,"\n The CURENT LOG_LEVEL IS                  : %d", log_level);
		log_info(4,"\n The STRATEGY IS                     	    : %s", strategy);		
		log_info(4,"\n The TOTAL UP_TIME IS                	    : %.f seconds, %ld nano Seconds", seconds.tv_sec, seconds.tv_nsec);
		log_info(4,"\n ********------------------------------------------------------------******** ");
	}
	else{
 		log_info(4,"\n *** INFORMATION REGARDING ENVIRONMENTAL AND STATIC VARIABLES ***\n");
 		log_info(4,"\n The BOUNDED BUFFER SIZE IS          	    : %d", bSIZE);
 		log_info(4,"\n The THREAD POOL SIZE IS             	    : %d", worker_number);
 		log_info(4,"\n The PORT NUMBER IS                  	    : %d", port_number);
 		log_info(4,"\n The DOCUMENT ROOT IS                	    : %s", root_path);
 		log_info(4,"\n The CURRENT LOG_LEVEL IS                 : %d", log_level);
 		log_info(4,"\n The STRATEGY IS                    	    : %s", strategy);
 		log_info(4,"\n The NUMBER OF REQUESTS HANDLED ARE  	    : %.f", no_request_handled);
 		log_info(4,"\n The TOTAL UP_TIME IS                	    : %ld seconds, %ld nano Seconds", seconds.tv_sec, seconds.tv_nsec);
 		log_info(4,"\n The TOTAL TIME SERVICING THE REQUEST IS  : %ld seconds, %ld ns", total_time_servicing_request_seconds.tv_sec, total_time_servicing_request_seconds.tv_nsec);
 
		if(no_request_handled)
		 avg_time_per_request.tv_nsec = total_time_servicing_request_seconds.tv_nsec/no_request_handled;

		log_info(4,"\n The TOTAL AMOUNT OF DATA TRANSFERED 	    : %.f bytes", total_data_transffered);
		log_info(4,"\n The AVERAGE TIME PER REQUEST IS     	    : %ld Nano seconds", avg_time_per_request.tv_nsec);
		log_info(4,"\n ********------------------------------------------------------------******** ");
	    }
}

int send_response(int fd, http_response_t *response) // send/print response header on console
{   
    FILE *f, *file_fd, *error_file_fd;
    size_t file_size, error_file_size;
    char buf[1024];
    char error_buf[1024];
    int i,n=0;

    f = fdopen(fd,"w");
    fprintf(f,"HTTP/%d.%d %d %s\r\n",response->major_version,response->minor_version,response->status.code,response->status.reason);

    for(i=0;i<response->header_count;i++)
    fprintf(f,"%s: %s\r\n",response->headers[i].field_name,response->headers[i].field_value);

    fprintf(f,"\n");
    
   { 
    file_fd = fopen(response->resource_path,"r");
    if(file_fd)
    {

    while(( file_size = fread(buf,1,sizeof(buf),file_fd))>0)  /* Write a response to the client */
    {        
    fwrite(buf,1,file_size,f);

    }
  
    fclose(file_fd);
    }	   
  }  
fclose(f);

return 0;
}

int build_response( http_request_t *request, http_response_t *response) //create response header for user request
{
    time_t now;
    char timebuf[MAX_HEADER_VALUE_LENGTH];
    char int_buffer[MAX_HEADER_VALUE_LENGTH];
    struct stat stat_buffer;
    response->header_count=0;
    int stat_flag =0;
    static int statusshow;
    FILE *f, *file_fd, *error_file_fd;
    size_t file_size, error_file_size;
    char buf[1024];
    char error_buf[1024];

    now = time(NULL);
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));//get systime for response in GMT

    response->major_version = request->major_version;
    response->minor_version = request->minor_version;

    if(request->method==HTTP_METHOD_GET)
    {	
    if (stat(request->uri,&stat_buffer)== -1) 
    {
    if (errno == EACCES) //check error number for access permisson 
    {
    (*response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_FORBIDDEN].code;
    statusshow=HTTP_STATUS_FORBIDDEN;
    }
    else 
    {
    (*response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_FOUND].code;
    statusshow=HTTP_STATUS_NOT_FOUND;           
    } 
    }
    else 
    {
    if(S_ISDIR(stat_buffer.st_mode))
    {
    strcat(request->uri, "index.html");	

    }
    stat_flag =1;
    (*response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_OK].code;
    statusshow=HTTP_STATUS_OK;
    }
    }
    else
    {
    (*response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_IMPLEMENTED].code;
    statusshow=HTTP_STATUS_NOT_IMPLEMENTED;
    } 

    if(response->status.code==HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_FOUND].code)
    {	

    char error_directory[PATH_MAX];

    strcpy(error_directory, root_path);
    strcat(error_directory, "/404.html");
    error_file_fd = fopen(error_directory, "r");

    if(error_file_fd)
    {
    log_info(0,"404 found");
    strcpy(request->uri,error_directory);
    fclose(error_file_fd);
    }
    else if(!error_file_fd)
    {
    strcpy(error_directory, root_path); 
    strcat(error_directory, "/400.html");
    error_file_fd = fopen(error_directory, "r");
    if(error_file_fd)
    {
    strcpy(request->uri,error_directory);
    fclose(error_file_fd);
    }
    else	
    {
    error_file_fd = fopen("errors/404.html", "r");
    if(error_file_fd)
    {

    strcpy(request->uri,"errors/404.html");
    fclose(error_file_fd);
    }
    }

    }
    }

    if(lstat(request->uri,&stat_buffer)<0)
    perror("");

    (*response).status = HTTP_STATUS_LOOKUP[statusshow];
	

    strcpy(response->resource_path,request->uri);
    strcpy(response->headers[response->header_count].field_name,"Date");
    strcpy(response->headers[response->header_count++].field_value,timebuf);

    strcpy(response->headers[response->header_count].field_name,"Server");
    strcpy(response->headers[response->header_count++].field_value,"CSUC HTTP");

    strcpy(response->headers[response->header_count].field_name,"Content-Type");
    strcpy(response->headers[response->header_count++].field_value,get_file_type(request->uri));

    total_data_transffered = total_data_transffered + stat_buffer.st_size;
    snprintf(int_buffer,MAX_HEADER_VALUE_LENGTH,"%ld",stat_buffer.st_size);
    strcpy(response->headers[response->header_count].field_name,"Content-Length");
    strcpy(response->headers[response->header_count++].field_value,int_buffer);

    return 0;
}


int checkMethod(char *requestMethod) //check for requested nethod and return respective http method code 
{
if(strcmp(requestMethod,"GET"     ) == 0)       return HTTP_METHOD_GET;  
if(strcmp(requestMethod,"OPTIONS" ) == 0)       return HTTP_METHOD_OPTIONS;
if(strcmp(requestMethod,"HEAD"    ) == 0)       return HTTP_METHOD_HEAD;      
if(strcmp(requestMethod,"POST"    ) == 0)       return HTTP_METHOD_POST;
if(strcmp(requestMethod,"PUT"     ) == 0)       return HTTP_METHOD_PUT;
if(strcmp(requestMethod,"DELETE"  ) == 0)       return HTTP_METHOD_DELETE;
if(strcmp(requestMethod,"TRACE"   ) == 0)       return HTTP_METHOD_TRACE;
if(strcmp(requestMethod,"CONNECT" ) == 0)       return HTTP_METHOD_CONNECT;

return HTTP_METHOD_NOT_FOUND;     
}

void print_log_level_information()
{
 print_log_level_info();
}

void handle_signals()
{

struct sigaction sa;
   
    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = 0;
    sa.sa_handler = graceful_shutdown;


struct sigaction sa_usr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags   = 0;
    sa_usr1.sa_handler = print_Env_and_stat_variables;

struct sigaction sa_usr2;
    sigemptyset(&sa_usr2.sa_mask);
    sa_usr2.sa_flags   = 0;
    sa_usr2.sa_handler = print_log_level_information;
	
    
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        exit(EXIT_FAILURE);
    }
    
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa_usr2, NULL) == -1) {
        exit(EXIT_FAILURE);
    }
    
}

int next_request(int fd, http_request_t *request) // function to save user input request header
{
FILE *f;
char path[URI_MAX];
char request_buffer[MAX_HEADER_LINE_LENGTH];
char headers_buffer[URI_MAX];
char header_name[MAX_HEADER_NAME_LENGTH];
char header_value[MAX_HEADER_VALUE_LENGTH];
int  major = 0,minor = 0;
int  no_of_str=0;
char method[10];
char *updatedString; 

    f = fdopen(fd,"r");
    if(f==NULL)return -1;

    fgets(request_buffer,MAX_HEADER_LINE_LENGTH,f);

    sscanf(request_buffer,"%s %s HTTP/%d.%d%*s",method,path,&major,&minor);

    while(request->header_count<MAX_HEADERS)
    {
    fgets(headers_buffer,MAX_HEADER_LINE_LENGTH,f);

    no_of_str = sscanf(headers_buffer, "%s %s",header_name,header_value);

    if(header_name[strlen(header_name)-1]==':')
    header_name[strlen(header_name)-1]='\0';

    if(no_of_str == 2)
    {
    strncpy(request->headers[request->header_count].field_name,header_name,MAX_HEADERS);
    strncpy(request->headers[request->header_count].field_value,header_value,MAX_HEADERS);
    request->header_count++;
    }
 
    if(no_of_str==EOF)
    break;
    }
    request->major_version = major;
    request->minor_version = minor;

    request->method = checkMethod(method);

    if(strstr(path, "?"))
    {
    strcpy(strpbrk(path, "?"),"");        
    }

    if(strstr(path, "#"))
    {
    strcpy(strpbrk(path, "?"),"");        
    }        

    strncat(request->uri, path, URI_MAX);

    return 0;

}

int response_clear_cache(http_response_t *response)
{
int i;
for(i=0;i<response->header_count;i++)
{
strcpy(response->headers[i].field_name,"");
strcpy(response->headers[i].field_value,"");
}
response->header_count = 0;
return 0;
}

void* using_Thread(void* arg)
{

	http_request_t *request;
	http_response_t *response;
	request  = (http_request_t *)malloc(sizeof(http_request_t));   // Memory allocation for request header
	response = (http_response_t *)malloc(sizeof(http_response_t)); // Memory allocation for response header
	strcpy(request->uri,root_path);
	int sock_fd = *((int *)arg); 
	int next_request_return_value = next_request(sock_fd, request);
	int build_ret_val = build_response(request,response);
	int send_response_ret_val = send_response(sock_fd,response);
	free(request);
	free(response);
	free((int*)arg);
    	pthread_exit(pthread_self);
}


int parse_command(int argc, char* argv[],int *port_number,int *activate_forking,int *activate_threading,int *multi_threading)
{
    struct stat dirbuf;
    int opt;

    while((opt = getopt(argc,argv,"p:d:w:q:v:ft"))!=-1)   //parsing command line arguments
    {
    switch(opt)
    {
    case 'p':
      *port_number = atoi(optarg);
      break;

    case 'd': 
      strcpy(root_path, optarg);
      break;
    case 'w':
      *multi_threading =1;
      worker_number = atoi(optarg);
      strategy = "THREAD_POOL";
    break;

    case 'q':
      bSIZE = atoi(optarg);
    break;

    case 'v':
      log_level = atoi(optarg);
    break;

    case 'f':
      strategy = "FORK";
      *activate_forking =1;	
      break;

    case 't':
      *activate_threading =1;
      strategy = "NORMAL_THREADING";
      break;

    default: 
      break;
    }
}

      if(stat(root_path, &dirbuf)== -1)
      {
     	  log_info(0,"No directory exist");
     	  return 0;
      }

     if(!(S_ISDIR(dirbuf.st_mode)))    //Checking for the right existing directory
     {
     	  log_info(0,"Requested Directory does not exist");
     	  return 0;
     }

     if((*activate_threading && *activate_forking) || (*multi_threading && *activate_forking) || (*multi_threading && *activate_threading))
     {
       log_info(0,"Too many arguments. Either use -t or -f 0r -w at a time");
       return 0;
     }
     if(!(*activate_threading) && !(*activate_forking) && !(*multi_threading))
	{
	  strategy = "SERIAL";
	 
	}
return 1;

}

void fun1(int port_number, int activate_forking, int activate_threading, int multi_threading)
{
struct sockaddr_in myaddr;
int cfd;
pid_t childId=0; 
int *ptr_cfd;

size_t nRequestSize = 1024;
memset(&myaddr,0,sizeof(struct sockaddr_in));
myaddr.sin_family = AF_INET;
myaddr.sin_addr.s_addr = INADDR_ANY;
myaddr.sin_port = htons(port_number);
int optval =1;

handle_signals();
   
lfd = socket(AF_INET,SOCK_STREAM,0);

if (lfd == -1)
perror("");

if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))
== -1)
log_info(0,"setsockopt");


if(bind(lfd, (struct sockaddr *) &myaddr, sizeof(struct sockaddr_in))==-1)
{
       log_info(0,"BIND FAILS");
       return ;
}

if(listen(lfd, BACKLOG)==-1)
log_info(0,"LISTEN");	
    	clock_gettime( CLOCK_REALTIME, &start_up);
if(multi_threading == 1)
	{       log_info(2,"\nEntered in Thread_Pool");
		
          
	  multithreading();
   	  
        }
else{
	
	
  	while(status==RUNNING)
	{	
 	 cfd = accept(lfd, (struct sockaddr *) NULL, NULL);

    	clock_gettime( CLOCK_REALTIME, &start); 

	if(cfd==-1)
	{
 	   // perror("server: accept");
            //close(lfd);
            //printf("\nmain socket fd closed:");
	 // log_info(1,"\nWARNING : Accept Failed/Interrupted:");
		continue;
	}

	else if(activate_threading == 1)
	{	
		ptr_cfd = malloc(sizeof(int));
  		pthread_t tid;
  		*ptr_cfd = cfd;
		pthread_mutex_lock(&(no_r_handled.mutex1));	
		no_request_handled = no_request_handled+1;	
		pthread_mutex_unlock(&(no_r_handled.mutex1));	
		pthread_create(&tid, NULL,using_Thread, ptr_cfd);
		pthread_detach(tid);
		clock_gettime( CLOCK_REALTIME, &stop);
		pthread_mutex_lock(&(accumulate.mutex2));
	   	total_time_servicing_request_seconds.tv_sec = total_time_servicing_request_seconds.tv_sec + abs( stop.tv_sec - start.tv_sec );
		total_time_servicing_request_seconds.tv_nsec= total_time_servicing_request_seconds.tv_nsec + abs( stop.tv_nsec - start.tv_nsec );
		pthread_mutex_unlock(&(accumulate.mutex2));

	}	

	else if(activate_forking == 1)
	{
          childId = fork();
 	  if(childId == 0)
 	  {
		no_request_handled = no_request_handled+1;
   	 	fun2(cfd);
		clock_gettime( CLOCK_REALTIME, &stop);
		pthread_mutex_lock(&(accumulate.mutex2));
	   	total_time_servicing_request_seconds.tv_sec = total_time_servicing_request_seconds.tv_sec + abs( stop.tv_sec - start.tv_sec );
		total_time_servicing_request_seconds.tv_nsec=total_time_servicing_request_seconds.tv_nsec + abs( stop.tv_nsec - start.tv_nsec );
		pthread_mutex_unlock(&(accumulate.mutex2)); 
        exit(0);
        }
 	  else
   	  {	
   	 	while(waitpid(-1,NULL,WNOHANG)>0);
   	 	close(cfd);
   	  }
	}


	else if((activate_threading ==0)  && (activate_forking == 0)) //Using Serial way
    	{	
		no_request_handled = no_request_handled+1;
		fun2(cfd);
		clock_gettime( CLOCK_REALTIME, &stop);
		pthread_mutex_lock(&(accumulate.mutex2));
		total_time_servicing_request_seconds.tv_sec = total_time_servicing_request_seconds.tv_sec + abs( stop.tv_sec - start.tv_sec );
		total_time_servicing_request_seconds.tv_nsec= total_time_servicing_request_seconds.tv_nsec + abs( stop.tv_nsec - start.tv_nsec );		
		pthread_mutex_unlock(&(accumulate.mutex2));
		close(cfd);	
    	}

       }
  }
	
	log_info(3,"\n***Main socket fd closed***");
	close(lfd);
}

int fun2(int cfd)
{
	http_request_t *request;
	http_response_t *response;
	request  = (http_request_t *)malloc(sizeof(http_request_t));   // Memory allocation for request header
	response = (http_response_t *)malloc(sizeof(http_response_t)); // Memory allocation for response header

	strcpy(request->uri,root_path);
	int next_request_return_value = next_request(cfd, request);
	if(next_request_return_value==-1) return -1; 
	int build_ret_val = build_response(request,response);
	
	if(build_ret_val==-1) return -1;
	int send_response_ret_val = send_response(cfd,response);

	response_clear_cache(response);
	free(request);
	log_info(3,"\nrequest memory freed");
	free(response);
	log_info(3,"\nresponse memory freed");
	return 0;
}

void* consumer1(void* arg)
{
	consumer();
}

int multithreading()
{
    int rc;
    static sigset_t old_signal_mask;
    int i;
    for ( i = 1; i < NUM_THREADS; i++)
    pthread_create(&tid[i], NULL, consumer1, NULL);
    rc = pthread_sigmask (SIG_SETMASK, &old_signal_mask,NULL );
    producer(NULL);	
    return 0;
}


int  main(int argc, char * argv[])
{
    strcpy(root_path,"./");
    port_number = 9000;
  
     int activate_threading=0, multi_threading=0,activate_forking=0;

    int parsing= parse_command(argc,argv,&port_number, &activate_forking, &activate_threading, &multi_threading);

    if(parsing==0)return 0;
    else fun1(port_number, activate_forking, activate_threading, multi_threading);
       
return 0;
}



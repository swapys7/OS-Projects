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
#include "csuc_http.h"
#include <pthread.h>

#define BACKLOG 10
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT" //Define time format in required GMT format
char root_path[PATH_MAX]="./";
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
		perror(response->resource_path);
		file_fd = fopen(response->resource_path,"r");
		if(file_fd)
		{
			perror("opened successfully");
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

char *get_file_type(const char *name) //check for file extension
{
	char *ext = strrchr(name, '.');
	if(!strstr(name,".")) return "text/html";

	if (strcmp(ext, ".html") == 0  || strcmp(ext, ".htm" ) == 0)  return "text/html";
	if (strcmp(ext, ".jpg" ) == 0  || strcmp(ext, ".jpeg") == 0)  return "image/jpeg";
	if (strcmp(ext, ".gif" ) == 0  || strcmp(ext, ".GIF" ) == 0)  return "image/gif";
	if (strcmp(ext, ".png" ) == 0  || strcmp(ext, ".PNG" ) == 0)  return "image/png";
	if (strcmp(ext, ".c"   ) == 0  || strcmp(ext, ".C"   ) == 0)  return "text/c";
	if (strcmp(ext, ".h"   ) == 0  || strcmp(ext, ".H"   ) == 0)  return "text/h";
	if (strcmp(ext, ".txt" ) == 0  || strcmp(ext, ".TXT" ) == 0)  return "text/txt";
	if (strcmp(ext, ".xml" ) == 0  || strcmp(ext, ".XML" ) == 0)  return "application/xml";
	if (strcmp(ext, ".js"  ) == 0  || strcmp(ext, ".JS"  ) == 0)  return "application/javascript";
	if (strcmp(ext, ".css" ) == 0  || strcmp(ext, ".CSS" ) == 0)  return "text/css";
	if (strcmp(ext, ".au"  ) == 0  || strcmp(ext, ".AU"  ) == 0)  return "audio/basic";
	if (strcmp(ext, ".wav" ) == 0  || strcmp(ext, ".WAV" ) == 0)  return "audio/wav";
	if (strcmp(ext, ".avi" ) == 0  || strcmp(ext, ".AVI" ) == 0)  return "video/x-msvideo";
	if (strcmp(ext, ".mpeg") == 0  || strcmp(ext, ".MPG" ) == 0)  return "video/mpeg";
	if (strcmp(ext, ".mp3" ) == 0  || strcmp(ext, ".MP3" ) == 0)  return "audio/mpeg";
	if (strcmp(ext, ".mp4" ) == 0  || strcmp(ext, ".MP4" ) == 0)  return "video/mp4";	
	if (strcmp(ext, ".mov" ) == 0  || strcmp(ext, ".MOV" ) == 0)  return "video/quicktime";	
	return "text/plain";
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
	else{
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
			perror("404 found");
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
	perror(response->resource_path);	

	strcpy(response->resource_path,request->uri);
	strcpy(response->headers[response->header_count].field_name,"Date");
	strcpy(response->headers[response->header_count++].field_value,timebuf);

	strcpy(response->headers[response->header_count].field_name,"Server");
	strcpy(response->headers[response->header_count++].field_value,"CSUC HTTP");

	strcpy(response->headers[response->header_count].field_name,"Content-Type");
	strcpy(response->headers[response->header_count++].field_value,get_file_type(request->uri));

	snprintf(int_buffer,MAX_HEADER_VALUE_LENGTH,"%ld",stat_buffer.st_size);
	strcpy(response->headers[response->header_count].field_name,"Content-Length");
	strcpy(response->headers[response->header_count++].field_value,int_buffer);
	return 0;
}


int checkMethod(char *requestMethod) //check for requested nethod and return respective http method code 
{
	if(strcmp(requestMethod,"GET"     )== 0)       return HTTP_METHOD_GET;  
	if(strcmp(requestMethod,"OPTIONS" )== 0)       return HTTP_METHOD_OPTIONS;
	if(strcmp(requestMethod,"HEAD"    )== 0)       return HTTP_METHOD_HEAD;      
	if(strcmp(requestMethod,"POST"    )== 0)       return HTTP_METHOD_POST;
	if(strcmp(requestMethod,"PUT"     )== 0)       return HTTP_METHOD_PUT;
	if(strcmp(requestMethod,"DELETE"  )== 0)       return HTTP_METHOD_DELETE;
	if(strcmp(requestMethod,"TRACE"   )== 0)       return HTTP_METHOD_TRACE;
	if(strcmp(requestMethod,"CONNECT" )== 0)       return HTTP_METHOD_CONNECT;

	return HTTP_METHOD_NOT_FOUND;     
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

int  main(int argc, char * argv[])
{
	struct sockaddr_in myaddr;
	int lfd, cfd, port_number=9000;
	int pid, ppid, opt;
	int activate_threading=0;
	int activate_forking=0;
    pid_t childId=0; 
    int *ptr_cfd;
	while((opt = getopt(argc,argv,"p:d:ft"))!=-1)   //parsing command line arguments
	{
		switch(opt)
		{
			case 'p':
				    port_number = atoi(optarg);
				    break;

			case 'd': 
				    strcpy(root_path, optarg);
               	    		    break;

			case 'f':
				    activate_forking =1;	
				    break;

			case 't':
				    activate_threading =1;
				    break;

			default: 
				    break;
		}
	}

       struct stat dirbuf;
       if(stat(root_path, &dirbuf)== -1)
       {
      	  perror("No directory exist");
      	  return 0;
       }

      if(!(S_ISDIR(dirbuf.st_mode)))    //Checking for the right existing directory
      {
      	  perror("Requested Directory does not exist");
      	  return 0;
      }

      if(activate_threading && activate_forking)
      {
	  perror("Too many arguments. Either use -t or -f at a time");
	  return 0;
      }
      else 
      if(activate_forking == 1)         //Using Fork
      {		
	  memset(&myaddr,0,sizeof(struct sockaddr_in));
	  myaddr.sin_family = AF_INET;
	  myaddr.sin_addr.s_addr = INADDR_ANY;
	  myaddr.sin_port = htons(port_number);
	  lfd = socket(AF_INET,SOCK_STREAM,0);

		if (lfd == -1)
		perror("");
		if(bind(lfd, (struct sockaddr *) &myaddr, sizeof(struct sockaddr_in))==-1)
        	{
          	 	perror("");
          		return 0;
        	}

		if(listen(lfd, BACKLOG)==-1)
		perror("LISTEN");

		while(1)
		{
  	 		cfd = accept(lfd, (struct sockaddr *) NULL, NULL);
	 		if(cfd==-1)
	 		{
	  			perror("");
	 		}
			else 
	  		childId = fork();
	  		if(childId == 0)
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
	    			free(response);
	    			return 0;
	   		}
	  		else
	   		{		
	    		 	while(waitpid(-1,NULL,WNOHANG)>0);
	    			close(cfd);
	   		}	
		}
     }
     else 
     if(activate_threading == 1)    // Using Threads
     {
		memset(&myaddr,0,sizeof(struct sockaddr_in));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = INADDR_ANY;
		myaddr.sin_port = htons(port_number);
    
		lfd = socket(AF_INET,SOCK_STREAM,0);

		if (lfd == -1)
		perror("");
	
		if(bind(lfd, (struct sockaddr *) &myaddr, sizeof(struct sockaddr_in))==-1)
		{
  		        perror("");
  		        return 0;
        }
		if(listen(lfd, BACKLOG)==-1)
		perror("LISTEN");

		while(1)
		{
	  		cfd = accept(lfd, (struct sockaddr *) NULL, NULL);
	  		if(cfd==-1)
	  		{
	    			perror("");
	  		}
	 		else 
	   		ptr_cfd = malloc(sizeof(int));
	   		pthread_t tid;
	   		*ptr_cfd = cfd;
	   		pthread_create(&tid, NULL,using_Thread, ptr_cfd);
           	pthread_detach(tid);
		}
     }
     else 
     if((activate_threading ==0)  && (activate_forking == 0)) //Using Serial way
     {
		memset(&myaddr,0,sizeof(struct sockaddr_in));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = INADDR_ANY;
		myaddr.sin_port = htons(port_number);
		lfd = socket(AF_INET,SOCK_STREAM,0);

		if (lfd == -1)
		perror("");
		if(bind(lfd, (struct sockaddr *) &myaddr, sizeof(struct sockaddr_in))==-1)
		{
          		perror("");
          		return 0;
       	}
		if(listen(lfd, BACKLOG)==-1)
		perror("LISTEN");
	
		while(1)
		{
			  cfd = accept(lfd, (struct sockaddr *) NULL, NULL);
			  if(cfd==-1)
			  {
				   perror("");
			  }
	  
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
			  free(response);
			  close(cfd);	       
		}
     }
close(lfd);       
return 0;
}

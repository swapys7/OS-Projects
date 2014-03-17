// http://localhost:PORT_NUMBER/sample.html

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

#define BACKLOG 10
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT" //Define time format in required GMT format

int send_response(int fd, http_response_t *response) // send/print response header on console
{   
        FILE *f, *file_fd;
        size_t file_size;
        char buf[1024];
        int i,n=0;

       
       f = fdopen(fd,"w");
        fprintf(f,"HTTP/%d.%d %d %s\r\n",response->major_version,response->minor_version,response->status.code,response->status.reason);

        for(i=0;i<response->header_count;i++)
           fprintf(f,"%s: %s\r\n",response->headers[i].field_name,response->headers[i].field_value);

      fprintf(f,"\n");   
        if(response->status.code==HTTP_STATUS_LOOKUP[HTTP_STATUS_OK].code)
        {
            printf("\n Requested string(path) is: %s",response->resource_path);
           
            file_fd = fopen(response->resource_path,"r");
       
            while(( file_size = fread(buf,1,sizeof(buf),file_fd))>0)  /* Write a response to the client */
    		{	
        	fwrite(buf,1,file_size,f);
        	}
	
	fclose(file_fd);
	}   
      
	fclose(f);
	
        return 0;
}

char *get_file_type(const char *name) //check for file extension
{
        char *ext = strrchr(name, '.');
        if (!ext) return NULL;
        if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
        if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(ext, ".gif") == 0) 	return "image/gif";
        if (strcmp(ext, ".png") == 0) 	return "image/png";
        if (strcmp(ext, ".c") == 0) 	return "text/c";
        if (strcmp(ext, ".h") == 0) 	return "text/h";
        if (strcmp(ext, ".txt") == 0) 	return "text/txt";
        if (strcmp(ext, ".xml") == 0) 	return "application/xml";
        if (strcmp(ext, ".js") == 0) 	return "application/javascript";
        if (strcmp(ext, ".css") == 0) 	return "text/css";
        return "Not Supported";
}

int build_response(const http_request_t *request, http_response_t *response) //create response header for user request
{
        time_t now;
        char timebuf[MAX_HEADER_VALUE_LENGTH];
        char int_buffer[MAX_HEADER_VALUE_LENGTH];
        struct stat stat_buffer;
        response->header_count=0;
        int stat_flag =0;
        static int statusshow;

        now = time(NULL);
        strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));//get systime for response in GMT

        response->major_version = request->major_version;
        response->minor_version = request->minor_version;

        if(request->method==1)
	{
			printf("\n Requested URI is->>: %s",request->uri);
                if (stat(request->uri,&stat_buffer)== -1) 
		{
                        if (errno == EACCES) //check error number for access permisson 
                        {
                                (*response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_FORBIDDEN].code;
                                statusshow=HTTP_STATUS_FORBIDDEN;
                       		
			}
                        else {
                                (*response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_FOUND].code;
                                statusshow=HTTP_STATUS_NOT_FOUND;;	   
                      		
                             } 
                }
                else {
                        stat_flag =1;
                        (*response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_OK].code;
                        statusshow=HTTP_STATUS_OK;
  	             }
        }
        else{
                (*response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_IMPLEMENTED].code;
                statusshow=HTTP_STATUS_NOT_IMPLEMENTED;
		
                        printf("\n statusshow =%d",statusshow);
                        printf("\nstatus code is:%d", (*response).status.code);
            } 
	(*response).status = HTTP_STATUS_LOOKUP[statusshow];
	
        strcpy(response->headers[response->header_count].field_name,"Date");
        strcpy(response->headers[response->header_count++].field_value,timebuf);

        strcpy(response->headers[response->header_count].field_name,"Server");
        strcpy(response->headers[response->header_count++].field_value,"CSUC HTTP");

        strcpy(response->headers[response->header_count].field_name,"Content-Type");
        strcpy(response->headers[response->header_count++].field_value,get_file_type(request->uri));
	
        if(stat_flag==1)
                snprintf(int_buffer,MAX_HEADER_VALUE_LENGTH,"%ld",stat_buffer.st_size);
	
        strcpy(response->headers[response->header_count].field_name,"Content-Length");
        strcpy(response->headers[response->header_count++].field_value,int_buffer);
        strcpy(response->resource_path,request->uri);
	perror("l163");

        return 0;
}


int checkMethod(char *requestMethod) //check for requested nethod and return respective http method code 
{
        if(strcmp(requestMethod,"GET")== 0)       return HTTP_METHOD_GET;  
        if(strcmp(requestMethod,"OPTIONS")== 0)   return HTTP_METHOD_OPTIONS;
        if(strcmp(requestMethod,"HEAD")== 0)      return HTTP_METHOD_HEAD;      
        if(strcmp(requestMethod,"POST")== 0)      return HTTP_METHOD_POST;
        if(strcmp(requestMethod,"PUT")== 0)       return HTTP_METHOD_PUT;
        if(strcmp(requestMethod,"DELETE")== 0)    return HTTP_METHOD_DELETE;
        if(strcmp(requestMethod,"TRACE")== 0)     return HTTP_METHOD_TRACE;
        if(strcmp(requestMethod,"CONNECT")== 0)   return HTTP_METHOD_CONNECT;

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
        int major = 0,minor = 0;
        int no_of_str=0;
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
			printf("\nrequest->header_count is: %d",request->header_count);                
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

int  main(const int argc,const char * argv[])
{
	
	if(argc != 3)
	{
	printf("\n ErrrOr!! Not enough arguments");
	return 1;
        }
	int lfd, cfd;
	int pid, ppid;
        socklen_t addrlen;
        struct sockaddr_in myaddr;
        http_request_t *request;
        http_response_t *response;

        memset(&myaddr,0,sizeof(struct sockaddr_in));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = INADDR_ANY;
        myaddr.sin_port = htons(atoi(argv[2]));

        request  = (http_request_t *)malloc(sizeof(http_request_t));   // Memory allocation for request header
        response = (http_response_t *)malloc(sizeof(http_response_t)); // Memory allocation for response header

        lfd = socket(AF_INET,SOCK_STREAM,0);

        if (lfd == -1)
                perror("");

        if(bind(lfd, (struct sockaddr *) &myaddr, sizeof(struct sockaddr_in))==-1)
                perror("");

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
                printf("\nConnection accepted!");

	pid_t childId = fork();
	printf("\nChild is: %d ", childId);
        if(childId == 0)
	{

	if(argc<2)
                strcpy(request->uri,"./");
        else
                strcpy(request->uri,argv[1]);
       	
        int next_request_return_value = next_request(cfd, request);
        if(next_request_return_value==-1) return -1; 
        int build_ret_val = build_response(request,response);
        if(build_ret_val==-1) return -1;

        int send_response_ret_val = send_response(cfd,response);
	
	response_clear_cache(response);
	return 0;
    }
	
	else
	{
	while(waitpid(-1,NULL,WNOHANG)>0);
	close(cfd);
	}
   }
        free(request);
        free(response);
        
	close(lfd);       
        return 0;
}



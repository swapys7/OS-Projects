#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include"csuc_http.h"

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT" //Define time format in required GMT format
#define PROTOCOL "HTTP/1.1"

char path[URI_MAX];
int method_type,size=0;
int process_return=1;
static int statusshow;

int send_response(FILE *fd, http_response_t **response) // send/print response header on console
{
  int i=0;
  char *payload_buffer,c;
     
   printf("%s %d %s\n",PROTOCOL,(**response).status.code,(**response).status.reason);

   for(i=0; i<4;i++)	{
     printf("%s%s\n",(**response).headers[i].field_name,(**response).headers[i].field_value);
    }
   printf("\n");
   if(size!=0){
	payload_buffer = malloc(sizeof(char)*(size+1));
    	fread(payload_buffer, size,1,fd); 
	for(i = 0;i < size;++i)
    	  printf("%c", ((char *)payload_buffer)[i]);
    } 
    else
	printf("\n No Payload\n");
   printf("\n");
  return;
}

char *get_file_type(char *name) //check for file extension
{
  char *ext = strrchr(name, '.');
  if (!ext) return NULL;
  if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
  if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
  if (strcmp(ext, ".gif") == 0) return "image/gif";
  if (strcmp(ext, ".png") == 0) return "image/png";
  if (strcmp(ext, ".c") == 0) return "text/c";
  if (strcmp(ext, ".h") == 0) return "text/h";
  if (strcmp(ext, ".txt") == 0) return "text/txt";
  return NULL;
}

int build_response(FILE *fd, http_request_t **request, http_response_t **response) //create response header for user request
{
     time_t now;
     char timebuf[MAX_HEADER_VALUE_LENGTH],temp_size[MAX_HEADER_VALUE_LENGTH];
     struct stat stat_buffer;
     
     now = time(NULL);
     strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));//get systime for response in GMT
	
     //save respective fields and values in response header    
     strcpy((**response).headers[0].field_name,"Date: ");
     strcpy((**response).headers[0].field_value,timebuf);

     strcpy((**response).headers[1].field_name,"Server: ");
     strcpy((**response).headers[1].field_value,"CSUC HTTP ");

     strcpy((**response).headers[2].field_name,"Content-Type: ");
     if(get_file_type((**request).uri)!= NULL)
     	   strcpy((**response).headers[2].field_value, get_file_type((**request).uri));
     strcpy((**response).headers[3].field_name,"Content-Length: ");

     if( stat(path,&stat_buffer)== -1)
           perror(NULL);
      	  size = stat_buffer.st_size; //get size for file
      	  sprintf(temp_size, "%d", size);

     if(method_type==1){
         if(process_return ==0){
	    (**response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_BAD_REQUEST].code;
           statusshow=HTTP_STATUS_BAD_REQUEST;
           }
		
	  else if (fd == NULL) {
      	    if (errno == EACCES) //check error number for access permisson 
	    {
	     (**response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_FORBIDDEN].code;
            statusshow=HTTP_STATUS_FORBIDDEN;
           }
           else {
            (**response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_FOUND].code;
            statusshow=HTTP_STATUS_NOT_FOUND;
	   } 
	}
    	else {
         (**response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_OK].code;
          statusshow=HTTP_STATUS_OK;
      	   strcpy((**response).headers[3].field_value,temp_size);
	}
     }
     else{
    	(**response).status.code = HTTP_STATUS_LOOKUP[HTTP_STATUS_NOT_IMPLEMENTED].code;
        statusshow=HTTP_STATUS_NOT_IMPLEMENTED;
	strcpy((**response).headers[3].field_value,temp_size);
     }
	
     (**response).status = HTTP_STATUS_LOOKUP[statusshow];
}

char *newStr(char* charBuffer) //remove '/' char from filename if exist
{
    if (charBuffer && (charBuffer[0] == '/'))
        return strdup(charBuffer + 1); 
    else 
        return strdup(charBuffer);
}

int tokenize(char* line,char* method,char* filename,char* protocol)//function to tokenize user input for request header 
{
   int i=0;
   char temp[100], *cmd;
      
   if(line != NULL) {
	cmd = strtok(line," "); //create token on occurrence of white space delimiter
	strcpy(method, cmd);
   }

   while(cmd != NULL)// continue tokenizing  till end of line or end of user input
   {
        cmd = strtok(NULL, " ");
	 
       if((i==0)&&(cmd != NULL)){
	   strcpy(temp, cmd);
	   strcpy(filename, newStr(temp)); 
	 }
	else if((i==1)&&(cmd != NULL)){
	   strcpy(protocol, cmd);
        }
	i++;
    } 
   return i;
}

int checkMethod(char *requestMethod) //check for requested nethod and return respective http method code 
{
     if(strcmp(requestMethod,"GET")== 0)
          return HTTP_METHOD_GET;  
     else if(strcmp(requestMethod,"OPTIONS")== 0)
          return HTTP_METHOD_OPTIONS;
     else if(strcmp(requestMethod,"HEAD")== 0)
          return HTTP_METHOD_HEAD;      
     else if(strcmp(requestMethod,"POST")== 0)
          return HTTP_METHOD_POST;
     else if(strcmp(requestMethod,"PUT")== 0)
          return HTTP_METHOD_PUT;
     else if(strcmp(requestMethod,"DELETE")== 0)
          return HTTP_METHOD_DELETE;
     else if(strcmp(requestMethod,"TRACE")== 0)
          return HTTP_METHOD_TRACE;
     else if(strcmp(requestMethod,"CONNECT")== 0)
          return HTTP_METHOD_CONNECT;
     else return -1;     
}

int processRequest(char* input, http_request_t **request) // function to save user input request header
{
   char method[32], filename[32], protocol[32], host[32], str[256], path[100],temp[100],c ;
   char *buff = malloc(URI_MAX);
   int cnt=0;

   strcpy (buff, input);
   cnt=tokenize(buff, method, filename, protocol); //call to function for string parsing
   if( cnt!=3)//check user has provided correct number of arguments for request header
      process_return=0;
   
  (**request).method = checkMethod(method);//save method name in request header
   strcpy ((**request).uri, filename);//save file name in request header
   method_type=checkMethod(method);//call to checkMethod to check for valid method
   
	return;
}

int  main(const int argc,const char * argv[])
{
   char *input = NULL,*input1=NULL,ch;
   size_t len = 0;
   ssize_t read;
   http_response_t *response;
   http_request_t *request;
   
   FILE *fd = NULL;

   request  = (http_request_t *)malloc(sizeof(http_request_t));   // Memory allocation for request header
   response = (http_response_t *)malloc(sizeof(http_response_t)); // Memory allocation for response header
          
   if (argc<2)
          strcpy(path,".");
     else  
          strcpy(path,argv[1]);
           
   read = getline(&input, &len, stdin); // Accept input for request header

   while(1) 
    {
	read = getline(&input1, &len, stdin); //To accpet more request headers  
	if(strstr(input1,":"))
	{
	}	
       else
	break;
     }
    
    processRequest(input, &request); // call to processRequest function for processing the request
	
    sprintf(path,"%s/%s",path,(*request).uri); // prints in system buffer
    fd = fopen(path, "r"); // open requested file in read mode

   //function call to response functions
    build_response(fd, &request, &response); 
    send_response(fd, &response);

    close(fd);
    free(input);
    exit(EXIT_SUCCESS);

return;
}

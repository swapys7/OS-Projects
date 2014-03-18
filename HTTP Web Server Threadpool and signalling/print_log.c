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
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include "csuc_http_swapniljoshi.h"


void log_info(int internal_log_level, const char *message, ...)
{
va_list args;
va_start(args,message);

switch(internal_log_level)
{
	case 0 : vfprintf(stderr,message,args);
		 break;

	case 1 : if(log_level>=1)
		 vfprintf(stderr,message,args);
		 break;

	case 2 : if(log_level>=2)
		 vfprintf(stdout,message,args);
		 //vfprintf(stderr,message,args);
		 break;

	case 3 : if(log_level==3)
		 vfprintf(stdout,message,args);
		 //vfprintf(stderr,message,args);
		 break;
	
	case 4 : vfprintf(stdout,message,args);//FOR SIGUSR1 ONLY. PRINT ALWAYS
		 break;
	
	default: break;
}
va_end(args);

}

void print_log_level_info()
{
	if(log_level==3)
	{
		log_level = 0;
		log_info(2,"\n Current Updated Log Level is: %d", log_level);
	}
	else
	{
 		log_level=log_level+1;
		log_info(2,"\n Current Updated Log Level is: %d", log_level);
	}
}





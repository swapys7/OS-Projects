#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>


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




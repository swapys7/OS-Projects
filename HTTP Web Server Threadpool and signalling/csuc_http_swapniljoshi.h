#ifndef _CSUC_HTTP_SWAPNILJOSHI_H
#define _CSUC_HTTP_SWAPNILJOSHI_H

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
#include <sys/time.h>
#include <stdarg.h>
#include "csuc_http.h"
#define NUM_THREADS 5
#define buff_SIZE 160
char root_path[PATH_MAX];
typedef struct {
  int *buf;
  int occupied;
  int nextin, nextout;
  pthread_mutex_t mutex;
  pthread_cond_t more;
  pthread_cond_t less;
} buffer_t;

typedef struct {
  pthread_mutex_t mutex1;
} buffer_t1;

typedef struct {
  pthread_mutex_t mutex2;
} buffer_t2;

typedef struct {
  pthread_mutex_t mutex3;
} buffer_t3;


 pthread_t tid[NUM_THREADS];

 buffer_t buffer;
 buffer_t1 no_r_handled;
 buffer_t2 accumulate;
 buffer_t3 TOTAL_UP_TIME;
 
 time_t current, later, request_later;
 long double request_seconds;
 double total_data_transffered;

 char *strategy;
 int lfd, worker_number, queue_size, log_level;
 int port_number;

 double no_request_handled;
 struct timespec start,start_up,stop,stop_sec, total_time_servicing_request_seconds, avg_time_per_request, accum_n, seconds;

enum {
    RUNNING,
    SHUTDOWN
};
volatile sig_atomic_t status;
#endif

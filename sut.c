#include "sut.h"
#include <pthread.h>
#include <ucontext.h>
#include<stdbool.h>
#include <unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include "./queue/queue.h"
#include "./YAUThreads/YAUThreads.h"
#include <time.h>
#include <string.h>
#include <fcntl.h>
//Creating the head of the file 
ucontext_t m;
pthread_t CEXEC;
pthread_t IEXEC;
struct queue ready_q;
struct queue wait_q;
struct queue job_q;
struct queue text_buffer;
struct queue reading_buffer; 
struct queue int_buffer;
int TotalNum_of_Thread = 0; 
int NumOf_Wait_Thread = 0; 
struct timespec second, nanosecond; 
//avoiding we modifie the ready queue at the same time
pthread_mutex_t Avoid_Double_Inserting = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t Avoid_IO_blocking = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t Avoid_job_q_blocking = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t Avoid_text_buffer_blocking = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t Avoid_reading_buffer_blocking = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t Avoid_int_blocking = PTHREAD_MUTEX_INITIALIZER;
bool ready_q_isempty = true;
bool first_task_complete = false;
bool wait_q_isempty = true;
int fp;
struct timespec remaining, request = {0, 100000};
void * routine(){
    pthread_mutex_lock(&Avoid_Double_Inserting);
    struct queue_entry *executing_task = queue_pop_head(&ready_q);
    pthread_mutex_unlock(&Avoid_Double_Inserting);
    while(!ready_q_isempty || !first_task_complete || executing_task || !wait_q_isempty){
        if (executing_task) {
            ucontext_t task = *(ucontext_t *)executing_task->data;
            swapcontext(&m, &task);
            first_task_complete = true;
        } else if (TotalNum_of_Thread == 0){
            ready_q_isempty = true;
        }
        pthread_mutex_lock(&Avoid_Double_Inserting);
        executing_task = queue_pop_head(&ready_q);
        pthread_mutex_unlock(&Avoid_Double_Inserting);
        nanosleep(&request, &remaining);
    }
    return 0;
}
void * routine2(){
    char current_job[8];
    char current_buffer[128];
    struct queue_entry *buffer_entry;
    struct queue_entry *job;
    struct queue_entry *write_int;
    pthread_mutex_lock(&Avoid_job_q_blocking);
    job = queue_pop_head(&job_q);
    pthread_mutex_unlock(&Avoid_job_q_blocking);
    while(!ready_q_isempty || !first_task_complete || job || !wait_q_isempty){
        if (job) {
            sprintf(current_job, "%s", (char*)(job ->data));
            if (strcmp(current_job, "open") == 0){
             pthread_mutex_lock(&Avoid_text_buffer_blocking);
             buffer_entry = queue_pop_head(&text_buffer);
             pthread_mutex_unlock(&Avoid_text_buffer_blocking);
             sprintf(current_buffer, "%s", (char*)(buffer_entry ->data));
             //printf("what we used to open file %s\n", current_buffer);
             fp = open(current_buffer, O_RDWR);
             //put the context associate with open back to the ready queue
             pthread_mutex_lock(&Avoid_IO_blocking);
             struct queue_entry *done_IO = queue_pop_head(&wait_q);
             pthread_mutex_unlock(&Avoid_IO_blocking);
             pthread_mutex_lock(&Avoid_Double_Inserting);
             queue_insert_tail(&ready_q, done_IO);
             pthread_mutex_unlock(&Avoid_Double_Inserting);
             NumOf_Wait_Thread = NumOf_Wait_Thread - 1;
             }  
             else if (strcmp(current_job, "write") == 0){
                 pthread_mutex_lock(&Avoid_text_buffer_blocking);
                 buffer_entry = queue_pop_head(&text_buffer);
                 pthread_mutex_unlock(&Avoid_text_buffer_blocking);
                 //printf("what we get from queue character by character: ");
                 //for (int i=0; i<30; i++) {
                  //   printf("%d ", *(char*)(buffer_entry ->data + i));
                 //}
                 //printf("what we get from queue %s \n", (char*)(buffer_entry ->data));
                 sprintf(current_buffer, "%s", (char*)(buffer_entry ->data));
                 pthread_mutex_lock(&Avoid_int_blocking);
                 write_int = queue_pop_head(&int_buffer);
                 pthread_mutex_unlock(&Avoid_int_blocking);
                 int a = *(int*)(write_int->data);
                 write(fp, current_buffer, a); 
                 pthread_mutex_lock(&Avoid_IO_blocking);
                 struct queue_entry *done_IO = queue_pop_head(&wait_q);
                 pthread_mutex_unlock(&Avoid_IO_blocking);
                 pthread_mutex_lock(&Avoid_Double_Inserting);
                 queue_insert_tail(&ready_q, done_IO);
                 pthread_mutex_unlock(&Avoid_Double_Inserting);
                 NumOf_Wait_Thread = NumOf_Wait_Thread - 1;
             } else if (strcmp(current_job, "close") == 0){
                 close(fp);
                 pthread_mutex_lock(&Avoid_IO_blocking);
                 struct queue_entry *done_IO = queue_pop_head(&wait_q);
                 pthread_mutex_unlock(&Avoid_IO_blocking);
                 pthread_mutex_lock(&Avoid_Double_Inserting);
                 queue_insert_tail(&ready_q, done_IO);
                 pthread_mutex_unlock(&Avoid_Double_Inserting);
                 NumOf_Wait_Thread = NumOf_Wait_Thread - 1;
             } else if (strcmp(current_job, "read") == 0) {
                 pthread_mutex_lock(&Avoid_int_blocking);
                 write_int = queue_pop_head(&int_buffer);
                 pthread_mutex_unlock(&Avoid_int_blocking);
                 int a = *(int*)(write_int->data);
                 char *c = (char *) calloc(a + 1, sizeof(char));
                 read(fp, c, a);
                 struct queue_entry *node = queue_new_node(c);
                 pthread_mutex_lock(&Avoid_reading_buffer_blocking);
                 queue_insert_tail(&reading_buffer, node);
                 pthread_mutex_unlock(&Avoid_reading_buffer_blocking);
                 pthread_mutex_lock(&Avoid_IO_blocking);
                 struct queue_entry *done_IO = queue_pop_head(&wait_q);
                 pthread_mutex_unlock(&Avoid_IO_blocking);
                 pthread_mutex_lock(&Avoid_Double_Inserting);
                 queue_insert_tail(&ready_q, done_IO);
                 pthread_mutex_unlock(&Avoid_Double_Inserting);
                 NumOf_Wait_Thread = NumOf_Wait_Thread - 1;
             }
        } else if (NumOf_Wait_Thread == 0){
            wait_q_isempty = true; 
        }
        pthread_mutex_lock(&Avoid_job_q_blocking);
        job = queue_pop_head(&job_q);
        pthread_mutex_unlock(&Avoid_job_q_blocking);
        nanosleep(&request, &remaining);
}  
}
void sut_init(){    
    //Create the queue
    wait_q = queue_create();
    ready_q = queue_create();
    job_q = queue_create();
    text_buffer = queue_create();
    reading_buffer = queue_create();
    int_buffer = queue_create();

    //Initialize the queue
    queue_init(&wait_q);
    queue_init(&ready_q);
    queue_init(&text_buffer);
    queue_init(&reading_buffer);
    queue_init(&job_q);
    queue_init(&int_buffer);

    //Initialize Number of the thread and Waiting thread
    TotalNum_of_Thread = 0; 
    NumOf_Wait_Thread = 0;
    ready_q_isempty = false;

    //Main Thread
    getcontext(&m);

    //Two Kernel Thread
    pthread_create(&CEXEC, NULL, routine, NULL);
    pthread_create(&IEXEC, NULL, routine2, NULL);   
}
bool sut_create(sut_task_f fn){
    threaddesc *t1; 
    ready_q_isempty = false;
    t1 = malloc(sizeof(threaddesc));
    getcontext(&(t1->threadcontext));
	t1->threadid = TotalNum_of_Thread;
    TotalNum_of_Thread = TotalNum_of_Thread + 1; 
	t1->threadstack = (char *)malloc(THREAD_STACK_SIZE);
	t1->threadcontext.uc_stack.ss_sp = t1->threadstack;
	t1->threadcontext.uc_stack.ss_size = THREAD_STACK_SIZE;
	t1->threadcontext.uc_link = &m;
	t1->threadcontext.uc_stack.ss_flags = 0;
	t1->threadfunc = &fn;
    makecontext(&(t1->threadcontext), fn, 0);
    struct queue_entry *node = queue_new_node(&(t1->threadcontext));
    pthread_mutex_lock(&Avoid_Double_Inserting);
    queue_insert_tail(&ready_q, node);
    pthread_mutex_unlock(&Avoid_Double_Inserting);
    return true;
}
int count = 0;
void sut_yield(){
    ucontext_t current_task;
    getcontext(&current_task);
    struct queue_entry *node = queue_new_node(&current_task);
    pthread_mutex_lock(&Avoid_Double_Inserting);
    queue_insert_tail(&ready_q, node);
    pthread_mutex_unlock(&Avoid_Double_Inserting);
    swapcontext(&current_task, &m);
}
void sut_exit(){
    ucontext_t current_task;
    getcontext(&current_task);
    TotalNum_of_Thread = TotalNum_of_Thread - 1;
    swapcontext(&current_task, &m);
}
int sut_open(char *dest){
    ucontext_t current_task;
    getcontext(&current_task);
    struct queue_entry *node = queue_new_node(&current_task);
    //Insert The Current Job
    pthread_mutex_lock(&Avoid_IO_blocking);
    queue_insert_tail(&wait_q, node);
    pthread_mutex_unlock(&Avoid_IO_blocking);
    NumOf_Wait_Thread = NumOf_Wait_Thread + 1;
    wait_q_isempty = false;
    char* buff = malloc(sizeof(dest) + 1);
    strcpy(buff, dest);
    char* work = "open";
    struct queue_entry *node3 = queue_new_node(buff);
    struct queue_entry *node2 = queue_new_node(work);

    pthread_mutex_lock(&Avoid_job_q_blocking);
    queue_insert_tail(&job_q, node2);
    pthread_mutex_unlock(&Avoid_job_q_blocking);

    pthread_mutex_lock(&Avoid_text_buffer_blocking);
    queue_insert_tail(&text_buffer, node3);
    pthread_mutex_unlock(&Avoid_text_buffer_blocking);

    swapcontext(&current_task, &m);
    return fp;
}
void sut_write(int fd, char *buf, int size){
    ucontext_t current_task;
    getcontext(&current_task);
    struct queue_entry *node = queue_new_node(&current_task);
    //Insert The Current Job
    pthread_mutex_lock(&Avoid_IO_blocking);
    queue_insert_tail(&wait_q, node);
    pthread_mutex_unlock(&Avoid_IO_blocking);

    NumOf_Wait_Thread = NumOf_Wait_Thread + 1;
    wait_q_isempty = false;
    char* buff = malloc(sizeof(buf)*size);
    strcpy(buff, buf);
    char* work = "write";
    struct queue_entry *node5 = queue_new_node(buff);
    struct queue_entry *node4 = queue_new_node(work);

    pthread_mutex_lock(&Avoid_job_q_blocking);
    queue_insert_tail(&job_q, node4);
    pthread_mutex_unlock(&Avoid_job_q_blocking);

    pthread_mutex_lock(&Avoid_text_buffer_blocking);
    queue_insert_tail(&text_buffer, node5);
    pthread_mutex_unlock(&Avoid_text_buffer_blocking);

    //size queue
    struct queue_entry *node6 = queue_new_node(&size);
    pthread_mutex_lock(&Avoid_int_blocking);
    queue_insert_tail(&int_buffer, node6);
    pthread_mutex_unlock(&Avoid_int_blocking);

    swapcontext(&current_task, &m);
}
void sut_close(int fd){
    ucontext_t current_task;
    getcontext(&current_task);
    struct queue_entry *node = queue_new_node(&current_task);
    //Insert The Current Job
    pthread_mutex_lock(&Avoid_IO_blocking);
    queue_insert_tail(&wait_q, node);
    pthread_mutex_unlock(&Avoid_IO_blocking);

    NumOf_Wait_Thread = NumOf_Wait_Thread + 1;
    wait_q_isempty = false;
    char* work = "close";
    struct queue_entry *node2 = queue_new_node(work);
    pthread_mutex_lock(&Avoid_job_q_blocking);
    queue_insert_tail(&job_q, node2);
    pthread_mutex_unlock(&Avoid_job_q_blocking);
    swapcontext(&current_task, &m);
}
char *sut_read(int fd, char *buf, int size){
     ucontext_t current_task;
    getcontext(&current_task);
    struct queue_entry *node = queue_new_node(&current_task);
    //Insert The Current Job
    pthread_mutex_lock(&Avoid_IO_blocking);
    queue_insert_tail(&wait_q, node);
    pthread_mutex_unlock(&Avoid_IO_blocking);

    NumOf_Wait_Thread = NumOf_Wait_Thread + 1;
    wait_q_isempty = false;
    char* work = "read";
    struct queue_entry *node2 = queue_new_node(work);
    pthread_mutex_lock(&Avoid_job_q_blocking);
    queue_insert_tail(&job_q, node2);
    pthread_mutex_unlock(&Avoid_job_q_blocking);

    struct queue_entry *node4 = queue_new_node(&size);
    pthread_mutex_lock(&Avoid_int_blocking);
    queue_insert_tail(&int_buffer, node4);
    pthread_mutex_unlock(&Avoid_int_blocking);

    swapcontext(&current_task, &m);
    pthread_mutex_lock(&Avoid_reading_buffer_blocking);
    struct queue_entry *node3 = queue_pop_head(&reading_buffer);
    pthread_mutex_unlock(&Avoid_reading_buffer_blocking);
    sprintf(buf, "%s", (char*)(node3 ->data));
    return buf; 
}
void sut_shutdown(){
    pthread_join(CEXEC, NULL);
    pthread_join(IEXEC, NULL);
}
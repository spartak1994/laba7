#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
struct thread_attrs{
pthread_mutex_t *mutex_ptr;
pthread_cond_t *cond_ptr;
int shmID;
};
void getTime(struct tm *current_time)
{
time_t sec = 0;
time(&sec);
*current_time = *(localtime(&sec));
}
void *thread_func(void *attr)
{
struct thread_attrs *data;
data = (struct thread_attrs *)attr;
pthread_mutex_lock( data->mutex_ptr );
sleep(3);
void *addr;
addr = shmat(data->shmID, NULL, 0);
if( addr == (void *)(-1) ){
printf("Can't attach SR MEM.\n");
exit(-1);
}
struct tm time;
getTime(&time);
printf("Time in second thread %d:%d:%d\n",time.tm_hour, time.tm_min, time.tm_sec);
*( (struct tm *)addr ) = time;
pthread_mutex_unlock( data->mutex_ptr );
pthread_cond_signal( data->cond_ptr );
}
int main(int argc, char const *argv[])
{
struct tm time;
int shmid;
void *addr;
pthread_t tid;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
if( pthread_mutex_init(&mutex, NULL) != 0 ){
printf("Can't create mutex.\n");
return -1;
}
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;
if( pthread_cond_init(&cond_var, NULL) != 0 ){
printf("Can't create condition variable.\n");
return -1;
}
shmid = shmget(IPC_PRIVATE, sizeof(struct tm), IPC_CREAT | IPC_EXCL | 0666);
if(shmid == -1){
printf("Can't create SR MEM.\n");
return -1;
}
addr = shmat(shmid, NULL, 0);
if( addr == (void *)(-1) ){
printf("Can't attach SR MEM.\n");
return -1;
}
struct thread_attrs attr;
attr.mutex_ptr = &mutex;
attr.cond_ptr = &cond_var;
attr.shmID = shmid;
if( pthread_create(&tid, NULL, thread_func, &attr) == 0 ){
getTime(&time);
printf("Time in main thread %d:%d:%d\n", time.tm_hour, time.tm_min, time.tm_sec);
sleep(1);
if( pthread_cond_wait(&cond_var, &mutex) == 0 ){
struct tm time_after_waiting;
getTime( &time_after_waiting );
printf("Was waiting for %d sec.\n", 3600*(time_after_waiting.tm_hour - time.tm_hour) + 60*(time_after_waiting.tm_min - time.tm_min) + time_after_waiting.tm_sec - time.tm_sec );
time = *( (struct tm *)addr );
printf("Time from SR MEM %d:%d:%d\n", time.tm_hour, time.tm_min, time.tm_sec);
}
else {
printf("ERROR!\n");
return -1;
}
shmdt(addr);
}
else{
printf("Can't create thread.\n");
return -1;
}
return 0;
}

// This routine does dummy memory operations to make cache completely dirty
// Change system parameters according to your system
// Compile with -lpthread option. Example: g++ -o flush_cahce flush_cache.cpp -lpthread
//
// LLC_SIZE :
// Size of last level cache. Total size of data matrix equals to size of last level cache
// to assure that all cache is occupied by this routine.
//
// BLOCK_SIZE :
// Matrix is partitioned in the size of cache blocks to minimize cost of cache coherency
// protocols. Each consecutive cache block is claimed by a different thread which allows
// both homogeneous data distribution among threads and guarantees that two different threads
// does not work on the same cache block.
//
// NUM_OF_CORES :
// Routine generates 1 thread for each core and sets affinity of threads to individual cores 
// to avoid reducing private cache performance. 
//
// If you have suggestions, please share it with me

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <pthread.h>
#include <stdint.h>
#include <math.h>
#include <xmmintrin.h>
#include <iomanip>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>

#define BLOCK_SIZE 64 // 1 cache block has 64 bytes
#define LLC_SIZE 2048 // Last level cache size: 2048K = 2MB
#define NUM_OF_CORES 1
#define MILLION 1000000L
//#define LOG

using namespace std;

uint8_t datamatrix [(LLC_SIZE*1024)/(NUM_OF_CORES*BLOCK_SIZE)][NUM_OF_CORES][BLOCK_SIZE];

pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;
std::pair<rusage,rusage> proc_stats [NUM_OF_CORES];

class thread_args {
public:
    int thread_id;
};

int stick_this_thread_to_core(int core_id);

void * mess_with_cache(void *arg){
  // get thread id
  thread_args * args = (thread_args *) arg;
  int thread_id = args->thread_id;
  
  // set thread affinity
  if (stick_this_thread_to_core(thread_id) != 0){
    pthread_mutex_lock(&cout_mutex);
    cout << "Thread " << thread_id << " couldn't be assigned!"<< endl; 
    pthread_mutex_unlock(&cout_mutex);
  }
  #ifdef LOG
  else {
    pthread_mutex_lock(&cout_mutex);
    cout << "Thread " << thread_id << " is assigned to core " << thread_id << "!"<< endl; 
    pthread_mutex_unlock(&cout_mutex);
  }
  #endif

  getrusage(RUSAGE_SELF,&(proc_stats[thread_id].first));
  // populate matrix with random values
  int cnt = 0;
  for (int i = 0 ; i < (LLC_SIZE*1024)/(NUM_OF_CORES*BLOCK_SIZE) ; i++ ){
    for (int j = 0 ; j < BLOCK_SIZE ; j++){
      datamatrix[i][thread_id][j] = rand()%256;
      cnt ++;
    }
  }
  
  // scan local part of matrix and find sum of elements
  uint64_t sum = 0;
  for (int i = 0 ; i < (LLC_SIZE*1024)/(NUM_OF_CORES*BLOCK_SIZE) ; i++ ){
    for (int j = 0 ; j < BLOCK_SIZE ; j++){
      sum += datamatrix[i][thread_id][j];
    }
  }  
  getrusage(RUSAGE_SELF,&(proc_stats[thread_id].second));
  
  // calculate statistics about resource usage
  uint64_t diff = MILLION * (proc_stats[thread_id].second.ru_utime.tv_sec - proc_stats[thread_id].first.ru_utime.tv_sec) + proc_stats[thread_id].second.ru_utime.tv_usec - proc_stats[thread_id].first.ru_utime.tv_usec;
  uint64_t UT_SEC  = diff / MILLION;
  uint64_t UT_USEC = diff % MILLION;
  diff = MILLION * (proc_stats[thread_id].second.ru_stime.tv_sec - proc_stats[thread_id].first.ru_stime.tv_sec) + proc_stats[thread_id].second.ru_stime.tv_usec - proc_stats[thread_id].first.ru_stime.tv_usec;
  uint64_t ST_SEC  = diff / MILLION;
  uint64_t ST_USEC = diff % MILLION;
  
  #ifdef LOG
  pthread_mutex_lock(&cout_mutex);
  cout << "Thread " << thread_id;
  cout << " , " << cnt;
  cout << " , " << sum;
  cout << " , " << UT_SEC << "." << setfill('0') << setw(6) << UT_USEC;
  cout << " , " << ST_SEC << "." << setfill('0') << setw(6) << ST_USEC;
  cout << " , " << (proc_stats[thread_id].second.ru_maxrss);//   - proc_stats[thread_id].first.ru_maxrss);
  cout << " , " << (proc_stats[thread_id].second.ru_ixrss);//    - proc_stats[thread_id].first.ru_ixrss); // not available on linux
  cout << " , " << (proc_stats[thread_id].second.ru_idrss);//    - proc_stats[thread_id].first.ru_idrss); // not available on linux
  cout << " , " << (proc_stats[thread_id].second.ru_isrss);//    - proc_stats[thread_id].first.ru_isrss); // not available on linux
  cout << " , " << (proc_stats[thread_id].second.ru_minflt   - proc_stats[thread_id].first.ru_minflt);
  cout << " , " << (proc_stats[thread_id].second.ru_majflt   - proc_stats[thread_id].first.ru_majflt);
  cout << " , " << (proc_stats[thread_id].second.ru_nswap    - proc_stats[thread_id].first.ru_nswap);
  cout << " , " << (proc_stats[thread_id].second.ru_inblock  - proc_stats[thread_id].first.ru_inblock);
  cout << " , " << (proc_stats[thread_id].second.ru_oublock  - proc_stats[thread_id].first.ru_oublock);
  cout << " , " << (proc_stats[thread_id].second.ru_msgsnd   - proc_stats[thread_id].first.ru_msgsnd);
  cout << " , " << (proc_stats[thread_id].second.ru_msgrcv   - proc_stats[thread_id].first.ru_msgrcv);
  cout << " , " << (proc_stats[thread_id].second.ru_nsignals - proc_stats[thread_id].first.ru_nsignals);
  cout << " , " << (proc_stats[thread_id].second.ru_nvcsw    - proc_stats[thread_id].first.ru_nvcsw);
  cout << " , " << (proc_stats[thread_id].second.ru_nivcsw   - proc_stats[thread_id].first.ru_nivcsw) << endl; 
  pthread_mutex_unlock(&cout_mutex);
  #endif
}


int main(int argc, char* argv[]) {
  pthread_t threads[NUM_OF_CORES];
  thread_args *th_args = (thread_args*) malloc((NUM_OF_CORES) * sizeof(thread_args));
  for (int i = 0 ; i < NUM_OF_CORES ; i++){
    th_args[i].thread_id = i;
    pthread_create(&threads[i], NULL, mess_with_cache, (void*) &th_args[i]); 
  }
    
  for (int i = 0 ; i < NUM_OF_CORES ; i++)
    pthread_join(threads[i], NULL);
}

int stick_this_thread_to_core(int core_id) {  
  if (core_id < 0)
    return -1;
  core_id = core_id % NUM_OF_CORES;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  pthread_t current_thread = pthread_self();    
  return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

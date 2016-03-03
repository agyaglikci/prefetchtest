/*
 * main.hpp
 *
 *  Created on: Feb 15, 2016
 *      Author: agy
 */

#ifndef MAIN_HPP_
#define MAIN_HPP_

#include <iostream>
#include <cstdint>
#include <xmmintrin.h>
#include <unistd.h>
#include <papi.h>
#include <time.h>
#include <pthread.h>

#define NUM_OF_CELLS 1000
#define NUM_OF_ITER  100
#define BULK_DATA_NUM_OF_ELEMENTS (64-sizeof(int)*3-sizeof(long))/sizeof(int)

struct __attribute__ ((packed))  data_cell {
	int index = 0;
	long pointer = 0;
	int next = 0;
	int touched = 0;
	int bulk_data [BULK_DATA_NUM_OF_ELEMENTS];
};

long long do_experiment (bool prefetch_enabled);
int factorial (int in);
void factorial_timing (int in);

// PAPI Related Definitions
#define NUM_OF_ANALYSIS 1
#define NUM_OF_COUNTERS 4

int ret;
long long temp_values [NUM_OF_COUNTERS];
long long counter_values[NUM_OF_ANALYSIS][NUM_OF_COUNTERS]; //Results are stored here!

int selected_counters [NUM_OF_COUNTERS] = { PAPI_TOT_CYC/*, PAPI_TOT_INS*/, PAPI_L1_DCM,PAPI_L1_DCA/*,PAPI_L2_DCM*/,PAPI_L2_DCA/* ,PAPI_TLB_DM*/ };
std::string cntr_name [NUM_OF_COUNTERS] = {"Total Cycles"/*, "Instr Completed"*/, "L1 Data Miss","L1 Data Acc"  /*,"L2 Data Miss"*/,"L2 Data Acc"/*, "Data TLB Miss"*/};

// for clock_gettime
struct timespec start, stop;
#define BILLION 1000000000L


// set affinity
int stick_this_thread_to_core(int core_id) {
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);
   pthread_t current_thread = pthread_self();
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}
#endif /* MAIN_HPP_ */

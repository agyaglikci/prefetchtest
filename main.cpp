/*
 * main.cpp
 *
 *  Created on: Feb 15, 2016
 *      Author: agy
 */

#include "main.hpp"

using namespace std;

int main (int argc, char* argv[]){
	factorial_timing(250);
//	long long sum1 = do_experiment(false);
//	long long sum2 = do_experiment(true);
//	cout << dec << sum1 << endl;
//	cout << dec << sum2 << endl;
	PAPI_start_counters(selected_counters, NUM_OF_COUNTERS);
	PAPI_stop_counters(temp_values,NUM_OF_COUNTERS);
	for(int c = 0 ; c < NUM_OF_COUNTERS ; c++) {
		cout << "[PAPI] " << cntr_name[c] << " : " << temp_values[c] << endl;
	}
	cout << "[PAPI] Time: " << temp_values[0] / 3.8 << "ns" << endl;

	PAPI_start_counters(selected_counters, NUM_OF_COUNTERS);
	factorial(250);
	PAPI_stop_counters(temp_values,NUM_OF_COUNTERS);
	for(int c = 0 ; c < NUM_OF_COUNTERS ; c++) {
		cout << "[PAPI] " << cntr_name[c] << " : " << temp_values[c] << endl;
	}
	cout << "[PAPI] Time: " << temp_values[0] / 3.8 << "ns" << endl;

	PAPI_start_counters(selected_counters, NUM_OF_COUNTERS);
	PAPI_stop_counters(temp_values,NUM_OF_COUNTERS);
	for(int c = 0 ; c < NUM_OF_COUNTERS ; c++) {
		cout << "[PAPI] " << cntr_name[c] << " : " << temp_values[c] << endl;
	}
	cout << "[PAPI] Time: " << temp_values[0] / 3.8 << "ns" << endl;


	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); /* mark start time */
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop); /* mark start time */
	cout << "[CLK_GET_TIME] Time: " <<  BILLION * (stop.tv_sec - start.tv_sec) + stop.tv_nsec - start.tv_nsec << " ns" << endl;

	return 0;
}

long long do_experiment (bool prefetch_enabled){
	int cpu_no = 1;
	int ret = stick_this_thread_to_core(cpu_no);
	if (ret == 0)
		cout << "experiment is set to CPU "<< cpu_no << endl;
	data_cell data_array [NUM_OF_CELLS];
	long long sum = 0;
	if (prefetch_enabled)
		cout << "Prefetch enabled" << endl;
	else
		cout << "Prefetch disabled" << endl;

	for ( int i = 0 ; i < NUM_OF_ITER ; i++ ){

		for ( int j = 0 ; j < NUM_OF_CELLS ; j++ ){
			int index = i*NUM_OF_CELLS+j;
			int next = (index + NUM_OF_CELLS/2)%NUM_OF_CELLS;
			data_array[j].index = index;
			data_array[j].next = next;
			data_array[j].pointer = (long) &(data_array[next]);
			data_array[j].touched = 0;
			for (int k = 0 ; k < BULK_DATA_NUM_OF_ELEMENTS; k++)
				data_array[j].bulk_data[k] = j + k;
		}

		// Clean the Caches
		system("./flush_cache");

		// Traverse the array (DFS)
		long long untouched_access_time = 0, touched_access_time = 0;
		long num_of_untouched_accesses = 0, num_of_touched_accesses = 0;
		int base_point = 0;
		int num_of_visited_cells = 0;
		int min_index = NUM_OF_ITER * NUM_OF_CELLS; // just to give traverser a purpose :)
//		PAPI_start_counters(selected_counters, NUM_OF_COUNTERS);
		while (num_of_visited_cells < NUM_OF_CELLS){
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); /* mark start time */
			int touched = data_array[base_point].touched; // Access to cell
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop); /* mark start time */
			if (touched == 0){
				if (prefetch_enabled)
					_mm_prefetch((void *) data_array[base_point].pointer , _MM_HINT_T0);
				untouched_access_time +=  BILLION * (stop.tv_sec - start.tv_sec) + stop.tv_nsec - start.tv_nsec ;
				num_of_untouched_accesses ++;
				if (data_array[base_point].index < min_index){
					min_index = data_array[base_point].index;
				}
				data_array[base_point].touched = 1;
				base_point = data_array[base_point].next;
				num_of_visited_cells ++;
				factorial(250);
			}
			else {
				touched_access_time +=  BILLION * (stop.tv_sec - start.tv_sec) + stop.tv_nsec - start.tv_nsec ;
				num_of_touched_accesses ++;
				base_point = (base_point+1)%NUM_OF_CELLS;
			}
		}
		for (int j = 0 ; j < NUM_OF_CELLS ; j++){
			for (int k = 0 ; k < BULK_DATA_NUM_OF_ELEMENTS; k++){
				//cout << data_array[j].bulk_data[k] << " ";
				sum += data_array[j].bulk_data[k];
			}
//			cout << endl;
		}
		cout << "Exp " << i << ":" 	<< " T:" << touched_access_time / (double)num_of_touched_accesses
									<< " U:" << untouched_access_time / (double)num_of_untouched_accesses << endl;

	}
	return sum;
}

int factorial (int a){
	int res = 1;
	for ( ; a > 0 ; a--){
		res *= a;
	}
	return res;
}

void factorial_timing (int in){
	long long average = 0;
	long long min = 1000000;
	long long max = 0;
	for (int i = 0 ; i < 1000000 ; i++){
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); /* mark start time */
		int a = factorial(in);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop); /* mark start time */
		long long point =  BILLION * (stop.tv_sec - start.tv_sec) + stop.tv_nsec - start.tv_nsec;
		if (point < min)
			min = point;
		if (point > max)
			max = point;
		average += point;
	}
	cout <<" F("<<in<<") : " <<  average / 1000000 << " e ["<<min<<","<<max<<"]"<<endl;
}

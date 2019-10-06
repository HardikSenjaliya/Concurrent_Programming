#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream> 
#include <set>
#include <algorithm> 
#include <vector> 
#include <iterator>
#include <pthread.h>
#include <bits/stdc++.h> 

#include "log.h"

#define MAX_BUCKETS			50

using namespace std;

multiset <int> bucket[MAX_BUCKETS];

/*Structure to store lower array_index upper array index*/
typedef struct {
	int lower_index, upper_index;
}array_index;

typedef struct{
	int max_key, max_threads;
	int lower_index, upper_index;
}bucket_data_t;

/*Global variables*/
pthread_barrier_t bar;
pthread_mutex_t write_bucket_lock;
int *input_ints = NULL;
struct timespec start_time, end_time;
/**
 * @brief this function sorts the given array
 * @param *array pointer to the given array
 * @param arraySize size of the given array
 * @return void
 * @ref: https://www.geeksforgeeks.org/merge-sort/
 */

void merge(int *array, int leftA_start, int leftA_end, int rightA_end){


    /* Merge Subarrays arr[leftA_start.. leftA_end] & arr leftA_end+1...rightA_end]*/
    int indexL, indexR, indexSorted;
    int leftA_n = leftA_end - leftA_start + 1;
    int rightA_n = rightA_end - leftA_end;

    /* create temp arrays */
    int LeftA[leftA_n], RightA[rightA_n];

    //Copy data to temp arrays L[] = left Subarray and R[] = right subarray 
    for (indexL = 0; indexL < leftA_n; indexL++)
        LeftA[indexL] = array[leftA_start + indexL];
    for (indexR = 0; indexR < rightA_n; indexR++)
        RightA[indexR] = array[leftA_end + 1 + indexR];

    /* Merge the temp arrays back into arr[l..r]*/
    indexL = 0;
    indexR = 0;
    indexSorted = leftA_start;
    while (indexL < leftA_n && indexR < rightA_n){
        if (LeftA[indexL] <= RightA[indexR]){
            array [indexSorted] = LeftA[indexL];
            indexL++;
        }
        else{
            array[indexSorted] = RightA[indexR];
            indexR++;
        }
            indexSorted++;
    }

    /* Copy the remaining elements of L[], if there are any */
    while (indexL < leftA_n){
        array[indexSorted] = LeftA[indexL];
        indexL++;
        indexSorted++;
    }

    /* Copy the remaining elements of R[], if there are any */
    while (indexR < rightA_n){
        array[indexSorted] = RightA[indexR];
        indexR++;
        indexSorted++;
    }
}

/*@brief This function divides the array into 
 * two parts recursively and calls merge function
 * to merge the sorted arrays
 *@lower_index pointer to first element in the array 
 *@upper_index pointer to last element in the array
 *@return void
 */
void merge_sort(int lower_index, int upper_index){

	if(lower_index < upper_index){
	
		int mid_index = lower_index + (upper_index - lower_index)/2;

		merge_sort(lower_index, mid_index);
		merge_sort(mid_index + 1, upper_index);

		merge(input_ints, lower_index, mid_index, upper_index);
	}
}

/*@brief This function is the thread function it divides the
 * array into parts and calls the merge function to merge sorted 
 * array
 *@params thread parameters 
 *@return null
 */
void *thread_function_merge(void *params){

	array_index *index = (array_index*)(params);

	int mid_index = index->lower_index + (index->upper_index - index->lower_index) / 2;

	if(index->lower_index < index->upper_index){
		
		merge_sort(index->lower_index, mid_index);
		merge_sort(mid_index + 1, index->upper_index);
		merge(input_ints, index->lower_index, mid_index, index->upper_index);
	}
	
	return NULL;
}

/*@brief this function divides the array based on the number of threads
 * assigns different parts of the array for each threads and then spawns 
 * threads
 *@array_size size of the input array
 *@num_of_threads number of threads to be used
 *@return void
 */
void initialize_threads_for_merge(int array_size, int num_of_threads){

	int low = 0;

	/*Initialize threads and barrier*/
	pthread_t threads[num_of_threads];
	//pthread_barrier_init(&bar, NULL, num_of_threads);

	/*Total size of the array is divided by the given number of threads 
	and each thread works on sorting the segment*/
	int array_seg_size = array_size / num_of_threads;
	array_index array_seg[num_of_threads];

	LOG(DBG, "Array size: %d Segment Size: %d", array_size, array_seg_size);

	for(int i = 1; i <= num_of_threads; i++){
		LOG(DBG, "Thread %d range ", i);
		array_seg[i].lower_index = low;

		if(i != num_of_threads){
			array_seg[i].upper_index = low + array_seg_size - 1;
			low += array_seg_size;
		}else{
			array_seg[i].upper_index = array_size - 1;
		}

		LOG(DBG, "Lower: %d Upper: %d", array_seg[i].lower_index, array_seg[i].upper_index);
	}

	/*only main thread works in case of just one threads*/
	clock_gettime(CLOCK_MONOTONIC,&start_time);
	if(num_of_threads == 1){
		thread_function_merge((void*)&array_seg[1]);
		clock_gettime(CLOCK_MONOTONIC,&end_time);
	}else{
		/*Spawn required number of threads to sort the array segments*/
		for(int i = 1; i <= num_of_threads; i++){
			if(pthread_create(&threads[i], NULL, thread_function_merge, (void*)&array_seg[i])){
				LOG(CRIT, "Spawning a thread, error code");
			}else{
				LOG(DBG, "Thread %d created successfully", i);
			}
		}

		clock_gettime(CLOCK_MONOTONIC,&end_time);

		for(int i = 1; i <= num_of_threads; i++){
			pthread_join(threads[i], NULL);
		}
	}

	/*All threads have sorted assigned array indexes, lets merge altogether*/
	for(int i = 1; i <= num_of_threads; i++){
		merge(input_ints, 0, array_seg[i].lower_index - 1, array_seg[i].upper_index);
	}

	unsigned long long elapsed_ns;
	elapsed_ns = (end_time.tv_sec-start_time.tv_sec)*1000000000 + (end_time.tv_nsec-start_time.tv_nsec);
	printf("Elapsed (ns): %llu\n",elapsed_ns);
	double elapsed_s = ((double)elapsed_ns)/1000000000.0;
	printf("Elapsed (s): %lf\n",elapsed_s);
}

/*@brief This is the thread function and each thread inserts the
 * array element into the bucket 
 *@params thread parameters 
 *@return null
 */
void *thread_function_bucket(void *params){

	

	bucket_data_t *buckets = (bucket_data_t*)(params);

	int insert_to_bucket = 0;	

	for(int index = buckets->lower_index; index <= buckets->upper_index; index++){

		insert_to_bucket = floor((buckets->max_threads * (*(input_ints + index)))/buckets->max_key);

		//LOG(DBG, "Insert to Bucket: %d", insert_to_bucket);
		pthread_mutex_lock(&write_bucket_lock);
		bucket[insert_to_bucket].insert(*(input_ints + index));
		pthread_mutex_unlock(&write_bucket_lock);
	}

	return NULL;
}

/*@brief this function divides the array based on the number of threads
 * assigns different parts of the array for each threads and then spawns 
 * threads
 *@array_size size of the input array
 *@num_of_threads number of threads to be used
 *@return void
 */
void initialize_threads_for_bucket(int array_size, int num_of_threads){

	LOG(DBG,"Bucket Sort");

	pthread_t threads[num_of_threads];
	
	if(pthread_mutex_init(&write_bucket_lock, NULL)){
		LOG(CRIT, "Unable to init mutex");
	}

	int low = 0;
	int array_seg_size = array_size / num_of_threads;

	int max_key = *max_element(input_ints, (input_ints + array_size));
	LOG(DBG, "Max Key: %d", max_key);

	bucket_data_t buckets[num_of_threads];

	for(int i = 1; i <= num_of_threads; i++){
		LOG(DBG, "Thread %d range ", i);
		buckets[i].lower_index = low;

		if(i != num_of_threads){
			buckets[i].upper_index = low + array_seg_size - 1;
			low += array_seg_size;
		}else{
			buckets[i].upper_index = array_size - 1;
		}

		buckets[i].max_key = max_key;
		buckets[i].max_threads = num_of_threads;
		LOG(DBG, "Lower: %d Upper: %d", buckets[i].lower_index, buckets[i].upper_index);
	}

	clock_gettime(CLOCK_MONOTONIC,&start_time);
	/*only main thread works in case of just one threads*/
	if(num_of_threads == 1){
		thread_function_bucket((void*)&buckets[1]);
		clock_gettime(CLOCK_MONOTONIC,&end_time);
	}
	else{
		/*Spawn required number of threads to sort the array segments*/
		for(int i = 1; i <= num_of_threads; i++){
			if(pthread_create(&threads[i], NULL, thread_function_bucket, (void*)&buckets[i])){
				LOG(CRIT, "Spawning a thread, error code");
			}else{
				LOG(DBG, "Thread %d created successfully", i);
			}
		}

		clock_gettime(CLOCK_MONOTONIC,&end_time);
		
		for(int i = 1; i <= num_of_threads; i++){
			pthread_join(threads[i], NULL);
		}
	}

	multiset <int> :: iterator itr;
	int index = 0;
	for(int i = 0; i <= num_of_threads; i++){
		//LOG(DBG, "Bucket %d", i);
		for(itr = bucket[i].begin(); itr != bucket[i].end(); itr++){
			//LOG(DBG, "%d", *itr);
			*(input_ints + index) = *itr;
			index++;
		}
	}

	unsigned long long elapsed_ns;
	elapsed_ns = (end_time.tv_sec-start_time.tv_sec)*1000000000 + (end_time.tv_nsec-start_time.tv_nsec);
	printf("Elapsed (ns): %llu\n",elapsed_ns);
	double elapsed_s = ((double)elapsed_ns)/1000000000.0;
	printf("Elapsed (s): %lf\n",elapsed_s);
}

/*@brief this function reads the input file and 
 * stores the input to an array dynamically
 *@input_file name of the input file to be read
 *@return number of the integers into the file
 */
int read_input_file(char *input_file){

	int count = 0, i = 0;

	/*Open input file to read the data, as number of input integers
	is unknown realloc is used to dynamically allocate memory and handle 
	any number of input integers*/
    FILE *pFile = fopen(input_file, "r");
    if(pFile == NULL){
    	LOG(CRIT, "Unable to open file");
    }
  	    
  	while (fscanf(pFile, "%d", &i) == 1){  
    	
  		if(input_ints == NULL){
  			input_ints = (int*)malloc(sizeof(int));
  			*input_ints = i;
  		}else{
  			count++;
  			input_ints = (int*)realloc(input_ints, sizeof(input_ints) * count);
  			if(input_ints != NULL){
  				*(input_ints + count) = i;
  			}else{
  				LOG(CRIT, "Unable to realloc memory");
  			}
  		}    
  	}

  	fclose(pFile);
  	return count + 1;
}

/*@brief this function prints the sorted array
 * into the file, if given, or prints on the 
 * stdout
 */

void save_sorted_array(char *output_file, int count){

    /*Print the sorted data to the output file, if provided via command line
    argument else print to stdout*/
  	if(output_file == NULL){
    	for(int i = 0; i < count; i++){
    		printf("%d\n", *(input_ints + i));
    	}
	}else{
    	FILE *pOFile = fopen(output_file, "w+");
    	if(pOFile == NULL){
    		LOG(CRIT, "Unable to Open Output File");
    	}

    	for(int i = 0; i < count; i++){
    		fprintf(pOFile, "%d\n", *(input_ints + i));
    	}

    	fclose(pOFile);
	}
}

/*
 * @brief the main function processes command line 
 * arguments and then calls the required function to 
 * sort the data
 */
int main(int argc, char** argv){

	int rc = 0, count = 0;
	
	char *input_file = NULL, *output_file = NULL, *sort_algo = NULL;
	int num_of_threads = 0;

	if(argc < 2){
		LOG(DBG,"Please provide atleast one argument");
		exit(-1);			
	}

	input_file = argv[1];

	static struct option long_options[] = {

		{"name", no_argument, 0, 'n'},
		{"input_file", required_argument, 0, 1},
		{"output_file", required_argument, 0, 'o'},
		{"threads", required_argument, 0, 't'},
		{"alg", required_argument, 0, 'a'},
		{0, 0, 0, 0}
	};
	
	/*Setting getopt_long for command line arguments --name and -o*/
	while((rc = getopt_long(argc, argv, ":o:t:a:", long_options, NULL)) != -1) {
		switch(rc){

			case 'n':
				printf("Hardik Senjaliya\n");
				exit(1);
				break;

			case 'o':
				output_file = optarg;
				LOG(DBG, "Output File Name: %s", output_file);
				break;

			case 't':
				num_of_threads = atoi(optarg);
				if(num_of_threads == 1 || num_of_threads == 0){
					num_of_threads = 2; // this makes sure work done by main thread only
				}

				LOG(DBG, "Num of threads: %d", num_of_threads);
				break;

			case 'a':
				sort_algo = optarg;
				LOG(DBG, "Sorting Algo: %s", sort_algo);

				break;
			case ':':
        		LOG(CRIT, "Missing Argument");
        		exit(-1);
                break;

            case '?':
            	LOG(CRIT, "Invalid Option");
            	exit(-1);
            	break;

    	    default:
    	    	LOG(CRIT, "Invalid Command line Argument");
            	exit(-1);
            	break;
		}
		
	}

	/*Read input file*/
	count = read_input_file(input_file);

	/*Check for the required algorithm and sort the data using 
		the given algorithm*/
  	if(!(strcmp(sort_algo, "merge"))){
  		initialize_threads_for_merge(count, num_of_threads - 1);
  	}else if(!(strcmp(sort_algo, "bucket"))){
  		initialize_threads_for_bucket(count, num_of_threads - 1);
  	}else{
  		LOG(CRIT, "Invalid Algorithm Selected");
  	}

	save_sorted_array(output_file, count);

  	free(input_ints);

   	return 0;
}
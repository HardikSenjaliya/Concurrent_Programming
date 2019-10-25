#include "mysort.h"
#include "log.h"

#define MAX_BUCKETS			50
#define SEQCST std::memory_order_seq_cst
#define RELAXED std::memory_order_relaxed

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


atomic<bool> my_tas_lock (false);
atomic<int> ticket_lock_next_num (0);
atomic<int> ticket_lock_now_serv (0);
void (*selected_lock_function)(int bucket, int index) = NULL;

class Node{

public:
	atomic<Node*> next;
	atomic<bool> wait;  
};

atomic<Node*> tail{NULL};

class MCSLock{

	public:

	void lock_aquire(Node* myNode){
		Node *oldTail = tail.load();
		myNode->next.store(NULL, RELAXED);

		while(!tail.compare_exchange_strong(oldTail, myNode)){
			oldTail = tail.load();
		}

		if(oldTail != NULL){
			myNode->wait.store(true, RELAXED);
			oldTail->next.store(myNode);
			while(myNode->wait.load()){};
		}

	}

	void lock_release(Node* myNode){
		Node* tempNode = myNode;
		if(tail.compare_exchange_strong(tempNode, NULL)){
			return;
		}else{
			while(myNode->next.load() == NULL){};
			myNode->next.load()->wait.store(false);
		}
	}

};

void test_and_set_lock(int ins_bucket, int index){
	//LOG(DBG, "Test and Set Lock");
	
	bool expected, changed;
	
	do{
		expected = false;
		changed = true;
	}while(!my_tas_lock.compare_exchange_strong(expected,changed));
		
	bucket[ins_bucket].insert(*(input_ints + index));
	
	my_tas_lock.store(false);
}

void test_and_test_and_set_lock(int ins_bucket, int index){
	bool expected, changed;
	do{
		expected = false;
		changed = true;
	}while(my_tas_lock.load()==true || !my_tas_lock.compare_exchange_strong(expected,changed));
	
	bucket[ins_bucket].insert(*(input_ints + index));
	
	my_tas_lock.store(false);
}

void ticket_lock(int ins_bucket, int index){
	int my_num = ticket_lock_next_num.fetch_add(1);
	while(ticket_lock_now_serv.load()!=my_num){}

	bucket[ins_bucket].insert(*(input_ints + index));
	
	ticket_lock_now_serv.fetch_add(1);
}

void MCS_lock(int ins_bucket, int index){

	// MCSLock t_lock;
	// Node t_node;
	// t_lock.lock_aquire(&t_node);
	// //counter++;
	// t_lock.lock_release(&t_node);
}

void pthread_lock(int ins_bucket, int index){
	//LOG(DBG, "Pthread Lock Function");
	pthread_mutex_lock(&write_bucket_lock);
	bucket[ins_bucket].insert(*(input_ints + index));
	pthread_mutex_unlock(&write_bucket_lock);
}


void (*p_lock_functions[])(int, int) = {
	test_and_set_lock,
	test_and_test_and_set_lock,
	ticket_lock,
	MCS_lock,
	pthread_lock	
};

const char *lock_names[] = {

	"tas",
	"ttas",
	"ticket",
	"mcs",
	"pthread"
};


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

		selected_lock_function(insert_to_bucket, index);
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

void print_usage(){
	LOG(DBG, "program_name inputfile.txt -t num_of_threads -i num_of_itr -o output.txt --lock [tas,ttas,ticket,mcs,pthread] --bar [sense,pthread]");
}

/*
 * @brief the main function processes command line 
 * arguments and then calls the required function to 
 * sort the data
 */
int main(int argc, char** argv){

	int rc = 0, count = 0;
	
	char *input_file = NULL, *output_file = NULL, *sort_algo = NULL, *lock = NULL, *bar = NULL;
	int num_of_threads = 0, num_of_itr = 0;

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
		{"itr", required_argument, 0, 'i'},
		{"bar", required_argument, 0, 'b'},
		{"lock", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};
	
	/*Setting getopt_long for command line arguments --name and -o*/
	while((rc = getopt_long(argc, argv, ":o:t:a:i:b:l:", long_options, NULL)) != -1) {
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

			case 'l':
				lock = optarg;
				LOG(DBG, "Lock: %s", lock);
				break;

			case 'i':
				num_of_itr = atoi(optarg);
				LOG(DBG, "Number of Itr: %d", num_of_itr);
				break;

			case 'b':
				bar = optarg;
				LOG(DBG, "Barrier: %s", bar);
				break;

			case ':':
        		LOG(CRIT, "Missing Argument for: %c", optopt);
        		print_usage();
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


	if(lock != NULL){
		if(!(strcmp(lock, lock_names[0]))){
			selected_lock_function = p_lock_functions[0];
		}else if(!(strcmp(lock, lock_names[1]))){
			selected_lock_function = p_lock_functions[1];
		}else if(!(strcmp(lock, lock_names[2]))){
		 	selected_lock_function = p_lock_functions[2];
		 }else if(!(strcmp(lock, lock_names[3]))){
		 	selected_lock_function = p_lock_functions[3];
		}else if(!(strcmp(lock, lock_names[4]))){
		 	selected_lock_function = p_lock_functions[4];
		 	if(pthread_mutex_init(&write_bucket_lock, NULL)){
				LOG(CRIT, "Unable to init mutex");
			}
		}else{
			LOG(CRIT, "Invalid Lock Selected");
			exit(-1);
		}
	}else{
		LOG(CRIT, "Invalid Lock Selected");
		exit(-1);
	}

	/*Check for the required algorithm and sort the data using 
		the given algorithm*/
  	if(!(strcmp(sort_algo, "bucket"))){
  		initialize_threads_for_bucket(count, num_of_threads - 1);
  	}else{
  		LOG(CRIT, "Invalid Algorithm Selected");
  	}

	save_sorted_array(output_file, count);

  	free(input_ints);

   	return 0;
}
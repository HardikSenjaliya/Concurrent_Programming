#include "counter.h"
#include "log.h"

using namespace std;

#define SEQCST std::memory_order_seq_cst
#define RELAXED std::memory_order_relaxed

#define MAIN_THREAD_ID				(0)


int counter = 0, num_of_threads = 0, num_of_itr = 0;
struct timespec start_time, end_time;
void (*selected_lock_function)(int) = NULL;
pthread_mutex_t counter_lock;
pthread_barrier_t counter_bar, timing_bar;

atomic<bool> my_tas_lock (false);
atomic<int> ticket_lock_next_num (0);
atomic<int> ticket_lock_now_serv (0);


class Barrier{

public:

	atomic<int> cnt;
	atomic<int> sense;
	int num_of_threads = 0;

	void wait();

	Barrier(){
		cnt = 0;
	}

}sense_bar;

void Barrier::wait(){

	thread_local bool my_sense = 0;

	if(my_sense == 0){
		my_sense = 1;
	}else{
		my_sense = 0;
	}

	int cnt_cpy = cnt.fetch_add(1);
	
	if(cnt_cpy == num_of_threads - 1){
		cnt.store(0, RELAXED);
		sense.store(my_sense);
	}else{
		while(sense.load() != my_sense){};
	}	
}


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

void test_and_set_lock(int tid){
	
	bool expected, changed;
	
	for(int i = 0; i < num_of_threads * num_of_itr; i++){
		if(i % num_of_threads == tid){

			do{
				expected = false;
				changed = true;
			}while(!my_tas_lock.compare_exchange_strong(expected,changed));
	
			counter++;
	
			my_tas_lock.store(false);
		}
	}

}

void test_and_test_and_set_lock(int tid){
	
	bool expected, changed;
	
	for(int i = 0; i < num_of_threads * num_of_itr; i++){
		if(i % num_of_threads == tid){

			do{
				expected = false;
				changed = true;
			}while(my_tas_lock.load()==true || !my_tas_lock.compare_exchange_strong(expected,changed));
	
			counter++;
	
			my_tas_lock.store(false);
		}
	}
}

void ticket_lock(int tid){
	
	for(int i = 0; i < num_of_threads * num_of_itr; i++){
		if(i % num_of_threads == tid){
			int my_num = ticket_lock_next_num.fetch_add(1);
			while(ticket_lock_now_serv.load()!=my_num){}
			counter++;
			ticket_lock_now_serv.fetch_add(1);
		}
	}

}

void MCS_lock(int tid){

	for(int i = 0; i < num_of_threads * num_of_itr; i++){
		if(i % num_of_threads == tid){

			MCSLock t_lock;
			Node t_node;

			t_lock.lock_aquire(&t_node);
			counter++;
			t_lock.lock_release(&t_node);
		}
	}
}

void pthread_lock(int tid){

	for(int i = 0; i < num_of_threads * num_of_itr; i++){
		if(i % num_of_threads == tid){

			pthread_mutex_lock(&counter_lock);
			counter++;
			pthread_mutex_unlock(&counter_lock);
		}
	}	

}


void sense_reverse(int tid){
	//LOG(DBG, "Sense Reversal Barrier");

	for(int i = 0; i < num_of_threads * num_of_itr; i++){
		if(i % num_of_threads == tid){
			counter++;
		}

		sense_bar.wait();	
	}

}

void pthread_bar(int tid){
	//LOG(DBG, "Pthread Barrier Function");

	for(int i = 0; i < num_of_threads * num_of_itr; i++){
		if(i % num_of_threads == tid){
			counter++;
		}

		pthread_barrier_wait(&counter_bar);
	}
}


void (*p_lock_functions[])(int) = {
	test_and_set_lock,
	test_and_test_and_set_lock,
	ticket_lock,
	MCS_lock,
	pthread_lock	
};


void (*p_bar_functions[])(int) = {
	sense_reverse,
	pthread_bar
};

void *counter_function(void *data){

	thread_data_t *t_data = (thread_data_t*)(data);

	pthread_barrier_wait(&timing_bar);
	if(t_data->tid == MAIN_THREAD_ID){
		clock_gettime(CLOCK_MONOTONIC,&start_time);
	}

	selected_lock_function(t_data->tid);

	pthread_barrier_wait(&timing_bar);
	if(t_data->tid == MAIN_THREAD_ID){
		clock_gettime(CLOCK_MONOTONIC,&end_time);
	}


	return NULL;
}

void spawn_threads(){

	pthread_t threads[num_of_threads];
	thread_data_t t_data[num_of_threads];

	/*For the main thread*/
	t_data[MAIN_THREAD_ID].tid = MAIN_THREAD_ID;
	//t_data[MAIN_THREAD_ID].num_of_itr = num_of_itr;
	//t_data[MAIN_THREAD_ID].num_of_threads = num_of_threads;

	for(int i = 1; i < num_of_threads; i++){
		t_data[i].tid = i;
		//t_data[i].num_of_itr = num_of_itr;
		//t_data[i].num_of_threads = num_of_threads;

		if(pthread_create(&threads[i], NULL, counter_function, (void*)&t_data[i])){
			LOG(CRIT, "Spawning a thread, error code ");
		}else{
			LOG(DBG, "Thread %d created successfully", i);
		}
	}

	/*For the main thread*/
	counter_function((void*)&t_data[MAIN_THREAD_ID]);

	for(int i = 1; i < num_of_threads; i++){
		pthread_join(threads[i], NULL);
	}

	LOG(DBG, "Counter Value: %d", counter);
}


/*@brief this function prints the sorted array
 * into the file, if given, or prints on the 
 * stdout
 */

void save_sorted_array(char *output_file){

    /*Print the sorted data to the output file, if provided via command line
    argument else print to stdout*/
  	if(output_file == NULL){
    	printf("%d\n", counter);
	}else{
    	FILE *pOFile = fopen(output_file, "w+");
    	if(pOFile == NULL){
    		LOG(CRIT, "Unable to Open Output File");
    	}

    	fprintf(pOFile, "%d\n", counter);

    	fclose(pOFile);
	}
}

void print_usage(){
	LOG(DBG, "Execution Instruction");
	LOG(DBG, "program_name -t num_of_threads -i num_of_itr -o output.txt --lock [tas,ttas,ticket,mcs,pthread]");
	LOG(DBG, "program_name -t num_of_threads -i num_of_itr -o output.txt --bar [sense,pthread]");
}

/*
 * @brief the main function processes command line 
 * arguments and then calls the required function to 
 * sort the data
 */
int main(int argc, char** argv){
	
	char *output_file = NULL, *lock = NULL, *bar = NULL;
	int rc = 0;

	if(argc < 2){
		LOG(DBG,"Please provide atleast one argument");
		exit(-1);			
	}

	static struct option long_options[] = {

		{"name", no_argument, 0, 'n'},
		{"output_file", required_argument, 0, 'o'},
		{"threads", required_argument, 0, 't'},
		{"itr", required_argument, 0, 'i'},
		{"bar", required_argument, 0, 'b'},
		{"lock", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};
	
	/*Setting getopt_long for command line arguments --name and -o*/
	while((rc = getopt_long(argc, argv, ":o:t:i:b:l:", long_options, NULL)) != -1) {
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
            	print_usage();
            	exit(-1);
            	break;

    	    default:
    	    	LOG(CRIT, "Invalid Command line Argument");
    	    	print_usage();
            	exit(-1);
            	break;
		}
	}

	if(lock != NULL && bar != NULL){
		print_usage();
		exit(-1);
	}

	pthread_barrier_init(&timing_bar, NULL, num_of_threads);

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
		 	if(pthread_mutex_init(&counter_lock, NULL)){
				LOG(CRIT, "Unable to init mutex");
			}
		}else{
			LOG(CRIT, "Invalid Lock Selected");
			exit(-1);
		}
	}else{

		if(!(strcmp(bar, bar_names[0]))){
			sense_bar.num_of_threads = num_of_threads;
			selected_lock_function = p_bar_functions[0];
		}else if(!(strcmp(bar, bar_names[1]))){
			pthread_barrier_init(&counter_bar, NULL, num_of_threads);
			selected_lock_function = p_bar_functions[1];
		}else{
			LOG(CRIT, "Invalid Barrier Selected");
			exit(-1);
		}
	}

	spawn_threads();

	save_sorted_array(output_file);
	
	unsigned long long elapsed_ns;
	elapsed_ns = (end_time.tv_sec-start_time.tv_sec)*1000000000 + (end_time.tv_nsec-start_time.tv_nsec);
	printf("Elapsed (ns): %llu\n",elapsed_ns);
	double elapsed_s = ((double)elapsed_ns)/1000000000.0;
	printf("Elapsed (s): %lf\n",elapsed_s);

	pthread_barrier_destroy(&timing_bar);
	pthread_mutex_destroy(&counter_lock);

   	return 0;
}
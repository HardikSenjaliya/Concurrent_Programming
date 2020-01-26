#include <stdlib.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <iostream> 
#include <iterator> 
#include <map> 

#include "main.h"
#include "log.h"

#define TRUE			1

using namespace std;

enum operations{
	CREATE,
	SEARCH,
	RANGE
};


/*Structure to store lower thread_data_t upper array index*/
typedef struct {
	uint16_t create_low_index, create_upper_index, search_low_index, 
	search_upper_index, range_lower_index, range_upper_index;
}thread_data_t;

/*Pointer to an array of node values to be inserted*/
int *node_values = NULL;
int *search_values = NULL;

/*Operation to be perfomed on a BST, 0xFF is the default value
which represents invalid operation, valid operation is selected 
by the user from the command line*/
int operation = 0xFF;

/*STL Map to store key:value pairs found in the given range*/
std::map<int, int> range_map;
pthread_mutex_t range_map_lock;

/*STL Map to store key:value pairs found in the search operation*/
std::map<int, int> search_map;
pthread_mutex_t search_map_lock;


/*Mutex to lock tree when the tree is empty*/
pthread_rwlock_t g_tree_lock;
bst_node *g_root = NULL;

/*variable to calculate time to perfrom the operation*/
struct timespec start_time, end_time;

/* @brief this function searches all the nodes between given range
 * all the key-value pairs found will be inserted into a STL map and 
 * will be printed once all threads completes execution
 * @params range_lower lower end of the given range
 * @params range_upper upper end of the given range
 * @params root base node of the BST
 */

void range_query(int range_lower, int range_upper, bst_node *root){
	
	if(root == NULL){
		// pthread_rwlock_rdlock(&g_tree_lock);
		// if(g_root == NULL){
		// 	pthread_rwlock_unlock(&g_tree_lock);
			return;
		// }

		// pthread_rwlock_rdlock(&g_root->lock);
		// root = g_root;
		// pthread_rwlock_unlock(&g_tree_lock);
	}
	

	if(range_lower < root->key){		

		if(root->left != NULL){
			pthread_rwlock_rdlock(&root->left->lock);
			pthread_rwlock_unlock(&root->lock);
			range_query(range_lower, range_upper, root->left);
			pthread_rwlock_rdlock(&root->lock);
		}
	}

	if(range_lower <= root->key && range_upper >= root->key){
		LOG(DBG, "Key found %d with value %d", root->key, root->value);
		pthread_mutex_lock(&range_map_lock);
		range_map.insert(pair<int, int>(root->key, root->value));
		pthread_mutex_unlock(&range_map_lock);
	}

	if(range_upper > root->key){
		
		if(root->right != NULL){
			pthread_rwlock_rdlock(&root->right->lock);
			pthread_rwlock_unlock(&root->lock);
			range_query(range_lower, range_upper, root->right);			
			pthread_rwlock_rdlock(&root->lock);
		}
	}

	pthread_rwlock_unlock(&root->lock);
}

/* @brief this function searches the given node in the BST
 * all the search values are inserted into a STL Map and will
 * be printed after all threads completes the execution
 * @params key given key to be searched in the BST
 * @params value future scope not used for now
 * @params root base node of the BST
 */

void search_node(int key, int value, bst_node *root){

	LOG(DBG, "Searching a Node with key %d", key);

	pthread_rwlock_rdlock(&g_tree_lock);
	
	if(g_root == NULL){

	 	LOG(DBG, "BST Empty or Node is Empty");
	 	pthread_rwlock_unlock(&g_tree_lock);
	 	return;
	}	

	pthread_rwlock_rdlock(&g_root->lock);
	root = g_root;
	pthread_rwlock_unlock(&g_tree_lock);

	while(TRUE){		
		if(key < root->key){		

			if(root->left == NULL){
				LOG(DBG, "Given Key not found");
				pthread_rwlock_unlock(&root->lock);
				return;
			}else{
				pthread_rwlock_rdlock(&root->left->lock);
				pthread_rwlock_unlock(&root->lock);
				root = root->left;
			}
		}else if(key > root->key){
			if(root->right == NULL){
				LOG(DBG, "Given Key not found");
				pthread_rwlock_unlock(&root->lock);
				return;
			}else{
				pthread_rwlock_rdlock(&root->right->lock);
				pthread_rwlock_unlock(&root->lock);
				root = root->right;
			}
		}else{
			LOG(DBG,"Key Found %d with Value %d", root->key, root->value);
			pthread_mutex_lock(&search_map_lock);
			search_map.insert(pair<int, int>(root->key, root->value));
			pthread_mutex_unlock(&search_map_lock);
			pthread_rwlock_unlock(&root->lock);
			return;
		}
	}
}

/* @brief this function creates a BST from the given Key & Value pair
 * @params key key of the node
 * @params value value of the related to given key
 * @params root base node of the BST
 */

void insert_node(int key, int value, bst_node *root){

	LOG(DBG, "Inserting New Node with key %d", key);

	pthread_rwlock_wrlock(&g_tree_lock); //lock the tree lock first to check valid tree exists
	
	if(g_root == NULL){ 		//if tree empty create a base node

	 	g_root = create_node(key, value);
	 	pthread_rwlock_unlock(&g_tree_lock);
	 	return;
	}	

	pthread_rwlock_wrlock(&g_root->lock);
	root = g_root;
	pthread_rwlock_unlock(&g_tree_lock);

	while(TRUE){
		if(key < root->key){

			if(root->left == NULL){
				root->left = create_node(key, value);
				pthread_rwlock_unlock(&root->lock);
				return;
			}else{
				pthread_rwlock_wrlock(&root->left->lock);
				pthread_rwlock_unlock(&root->lock);
				root = root->left;
			}
		}else if(key > root->key){
			if(root->right == NULL){
				root->right = create_node(key, value);
				pthread_rwlock_unlock(&root->lock);
				return;
			}else{
				pthread_rwlock_wrlock(&root->right->lock);
				pthread_rwlock_unlock(&root->lock);
				root = root->right;
			}
		}else{
			LOG(DBG,"Duplicate Not Allowed");
			pthread_rwlock_unlock(&root->lock);
			return;
		}
	}

	// if(root == NULL) {
	// 	pthread_rwlock_wrlock(&g_tree_lock);
		
	// 	if(g_root == NULL) {
	// 		g_root = create_node(key, value);
	//  		pthread_rwlock_unlock(&g_tree_lock);
	//  		return;
	// 	}

	// 	pthread_rwlock_wrlock(&g_root->lock);
	// 	root = g_root;
	// 	pthread_rwlock_unlock(&g_tree_lock);
	// }

	// if(key < root->key){
	// 	if(root->left == NULL){
	// 		root->left = create_node(key, value);
	// 		pthread_rwlock_unlock(&root->lock);
	// 	} else {
	// 		pthread_rwlock_wrlock(&root->left->lock);
	// 		pthread_rwlock_unlock(&root->lock);
	// 		insert_node(key, value, root->left);
	// 	}
	// }
	// else if(key > root->key) {
	// 	if (root->right == NULL) {
	// 		root->right = create_node(key, value);
	// 		pthread_rwlock_unlock(&root->lock);
	// 	} else {
	// 		pthread_rwlock_wrlock(&root->right->lock);
	// 		pthread_rwlock_unlock(&root->lock);
	// 		insert_node(key, value, root->right);
	// 	}
	// } else {
	// 	printf("Duplicates not allowed");
	// 	pthread_rwlock_unlock(&root->lock);
	// }

}

/* @brief this function creates a new node of the BST
 * @params key given key of the node
 * @params value value for the given key
 * @return bst_node pointer to the new BST node
 */
bst_node* create_node(int key, int val){

	bst_node *node = (bst_node *)malloc(sizeof(bst_node));

	if(node == NULL){
		LOG(CRIT, "Failed to create a new node");
		return NULL;
	}else{
		LOG(DBG, "New Node Created successfully with key %d", key);
	}

	node->key = key;
	node->value = val;
	node->left = NULL;
	node->right = NULL;
	pthread_rwlock_init(&node->lock, NULL);

	return node;
}

/* @brief this is the thread function, it takes care
 * of the function selected by the user along with creating
 * a BST, at a time either SEARCH & CREATE OR RANGE & CREATE
 * operations can be performed
 * @params params parameter of the thread function.
 */
void *thread_function(void *params){

	thread_data_t *index = (thread_data_t*)(params);

	/*starts inserting nodes in the specified range*/
	for(int i = index->create_low_index; i <= index->create_upper_index; i++){
		insert_node(*(node_values + i), (*(node_values + i))*2, g_root);
	}

	/*Switch between operation given by the user*/
	switch(operation){

		case SEARCH:
			for(int i = index->search_low_index; i <= index->search_upper_index; i++){
				search_node(*(search_values + i), (*(node_values + i))*2, g_root);
			}
		break;

		case RANGE:
			range_query(index->range_lower_index, index->range_upper_index, g_root);
		break;

		default:
		break;

	}

	return NULL;
}

/*@brief this function prints all the nodes 
 * of a BST
 *@params root base root of the SBT
 */

void traverse_inorder(bst_node *root){

	if(root != NULL){
		traverse_inorder(root->left);
		printf("\t %d", root->key);
		printf("\t %d\n", root->value);
		traverse_inorder(root->right);
	}
}

/*@brief this function prints all the nodes 
 * found during a search operation or a merge operation
 * @params map either reference to the map of stored search values
 * or stored range values
 */

void print_map(std::map<int, int> &map){

	printf("\nValues found KEY : VALUE\n");

    std::map<int, int>::iterator itr; 
    for (itr = map.begin(); itr != map.end(); ++itr) { 
       
         printf("\t %d", itr->first);
         printf("\t %d\n", itr->second);
    } 

}

/*@brief this function is the driver for all the operations
 * it divides the work between all the thread by dividing user
 * inputs for creating, searching nodes as well as for finding
 * all the nodes between a given range 
 * After diving the work, given number of threads will be spawned 
 * and will move further as all the thread joins.
 * At the end it prints time taken to perform requried operations
 * @params input is a pointer to the structure which stores
 * inputs given by the user 
 */
void spawn_threads(user_input *input){

	int num_of_threads = input->num_of_threads; //total number of threads
	int num_create_nodes = input->create_nodes; //total number of nodes to be created
	int num_search_nodes = input->search_nodes; //total number of nodes to be searched
	int range_lower = input-> range_lower; 		//lower end of the given range
	int range_upper = input->range_upper;  		//upper end of the given range
	int range = (range_upper - range_lower) + 1;

	/*Initialize global tree mutex*/
	pthread_rwlock_init(&g_tree_lock, NULL);

	pthread_t threads[num_of_threads];
	
	int low = 0;
	thread_data_t a_thread_data_t[num_of_threads];


	/*Total size of the nodes is divided by the given number of threads 
	and each thread works on inserting the nodes in a BST for eg if total nodes to
	be inserted is 100 and given number of threads is 5 then each thread will work
	on inserting 20 nodes in the BST*/
	int a_thread_data_t_size = num_create_nodes / num_of_threads;
				
	for(int i = 1; i <= num_of_threads; i++){
		a_thread_data_t[i].create_low_index = low;

		if(i != num_of_threads){
			a_thread_data_t[i].create_upper_index = low + a_thread_data_t_size - 1;
			low += a_thread_data_t_size;
		}else{
				a_thread_data_t[i].create_upper_index = num_create_nodes - 1;
		}

			//LOG(DBG, "Thraed %d Create Lower: %d Upper: %d", i, a_thread_data_t[i].create_low_index, a_thread_data_t[i].create_upper_index);
	}
	
	switch(operation){
	
		case SEARCH:{

			pthread_mutex_init(&search_map_lock, NULL);

			low = 0;

			/*Total size of the nodes is divided by the given number of threads 
			and each thread works on searching the elements in the segment for eg
			if the number of nodes to be searched is 100 and number of threads is 5 then 
			each thread will work on 20 nodes*/
			int a_thread_data_t_size = num_search_nodes / num_of_threads;
			

			for(int i = 1; i <= num_of_threads; i++){
				a_thread_data_t[i].search_low_index = low;

				if(i != num_of_threads){
					a_thread_data_t[i].search_upper_index = low + a_thread_data_t_size - 1;
					low += a_thread_data_t_size;
				}else{
					a_thread_data_t[i].search_upper_index = num_search_nodes - 1;
				}

				LOG(DBG, "Thread %d Search Lower: %d Upper: %d", i, a_thread_data_t[i].search_low_index, a_thread_data_t[i].search_upper_index);
			}

		}
		break;

		case RANGE:{

			pthread_mutex_init(&range_map_lock, NULL);

			int range_seg = range / num_of_threads;

			/*Total size of the nodes is divided by the given number of threads 
			and each thread works on searching the elements in the segment for eg
			if the given range is 1 to 100 and number of threads is 5 then each thread
			will work on finding nodes like 1st thread 1 to 20, 2nd thread 21 to 40 and so on...*/
			for(int i = 1; i <= num_of_threads; i++){
				a_thread_data_t[i].range_lower_index = range_lower;

				if(i != num_of_threads){
					a_thread_data_t[i].range_upper_index = range_lower + range_seg - 1;
					range_lower += range_seg;
				}else{
					a_thread_data_t[i].range_upper_index = range_upper;

				}

				LOG(DBG, "Thread %d RANGE : %d and %d", i, a_thread_data_t[i].range_lower_index, a_thread_data_t[i].range_upper_index);
			}

		}	
		break;

		default:
		break;
	}

	/*Record start time of the operations*/
	clock_gettime(CLOCK_MONOTONIC, &start_time);

	/*Create threads*/
	for(int i = 1; i <= num_of_threads; i++){

		if(pthread_create(&threads[i], NULL, thread_function, (void*)&a_thread_data_t[i])){
			LOG(CRIT, "Error while spawning a thread");
			exit(-1);
		}else{
			LOG(DBG, "Thread %d created successfully", i);
		}
	}

	/*Record end time of the operations*/
	clock_gettime(CLOCK_MONOTONIC, &end_time);

	for(int i = 1; i <= num_of_threads; i++){
		pthread_join(threads[i], NULL);
	}

	switch(operation){
		case SEARCH:
			printf("Selected Operation: SEARCH\n");
			print_map(search_map); // prints the values found
			free(search_values); //free values stored from the file
		break;

		case RANGE:
			print_map(range_map); // prints the values found
		break;
		
		default:
		break;

	}
	/*Prints time taken to perform the operation*/
	printf("\nTime Taken\n");
	unsigned long long elapsed_ns;
	elapsed_ns = (end_time.tv_sec-start_time.tv_sec)*1000000000 + (end_time.tv_nsec-start_time.tv_nsec);
	printf("Elapsed (ns): %llu\n",elapsed_ns);
	double elapsed_s = ((double)elapsed_ns)/1000000000.0;
	printf("Elapsed (s): %lf\n",elapsed_s);
}

/*@brief this function reads the input file and 
 * stores the input to an array dynamically
 *@testfile name of the input file to be read
 *@return number of the integers into the file
 */
int read_testfile(char *testfile, int **store_values){

	int count = 0, i = 0;

	/*Open input file to read the data, as number of input integers
	is unknown realloc is used to dynamically allocate memory and handle 
	any number of input integers*/
    FILE *pFile = fopen(testfile, "r");
    if(pFile == NULL){
    	LOG(CRIT, "Unable to open file");
    }
  	    
  	while (fscanf(pFile, "%d", &i) == TRUE){  
    	
  		if(*store_values == NULL){
  			*store_values = (int*)malloc(sizeof(int));
  			*(*store_values) = i;
  		}else{
  			count++;
  			*store_values = (int*)realloc(*store_values, sizeof(store_values) * count);
  			if(store_values != NULL){
  				*((*store_values + count)) = i;
  			}else{
  				LOG(CRIT, "Unable to realloc memory");
  			}
  		}    
  	}

  	fclose(pFile);
  	
  	return count + 1;
}

int main(int argc, char **argv){

	int rc = 0;
	char *selected_op = NULL, *create_file = NULL, *search_file = NULL;

	user_input input;
	memset(&input, 0, sizeof(input));

	if(argc < 2){
		LOG(DBG,"Please provide atleast one argument");
		exit(-1);			
	}

	create_file = argv[1];

	static struct option long_options[] = {

		{"help", no_argument, 0, 'h'},
		{"threads", required_argument, 0, 't'},
		{"ops", required_argument, 0, 'o'},
		{"search", required_argument, 0, 's'},
		{"range", required_argument, 0, 'r'},
		{"lower", required_argument, 0, 'l'},
		{"upper", required_argument, 0, 'u'},

		{0, 0, 0, 0}
	};
	
	/*Setting getopt_long for command line arguments --name and -o*/
	while((rc = getopt_long(argc, argv, ":t:o:s:r:l:u:h", long_options, NULL)) != -1) {
		switch(rc){

			case 'h':
				printf("Hardik Senjaliya\n");
				exit(1);
				break;

			case 't':
				input.num_of_threads = atoi(optarg);

				LOG(DBG, "Num of threads: %d", input.num_of_threads);
				break;

			case 'o':
				selected_op = optarg;
				LOG(DBG, "BST operation: %s", selected_op);
				break;

			case 's':
				search_file = optarg;
				LOG(DBG, "Search File: %s", search_file);
				break;

			case 'l':
				input.range_lower = atoi(optarg);
				LOG(DBG, "Range Lower Index: %d", input.range_lower);
				break;

			case 'u':
				input.range_upper = atoi(optarg);
				LOG(DBG, "Range Upper Index: %d", input.range_upper);
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

	/*Reads the file which has key values of the BST nodes*/
	input.create_nodes = read_testfile(create_file, &node_values);

	/*Check for the which operation was selected by the user*/
	if(!(strcmp(selected_op, "create"))){
		operation = CREATE;

		LOG(DBG, "Create Nodes Number: %d", input.create_nodes);

  	}else if(!(strcmp(selected_op, "search"))){
  		operation = SEARCH;

  		/*Reads the file which has key values to be searched in the BST*/
  		input.search_nodes = read_testfile(search_file, &search_values);

  		if(input.search_nodes < input.num_of_threads){
  			LOG(CRIT, "Number of threads %d is more than Nodes %d to be searched, keep it equal or less!", input.num_of_threads, input.search_nodes);
  			exit(-1);
  		}

		LOG(DBG, "Create Nodes Number: %d and Search Nodes Number: %d", input.create_nodes, input.search_nodes);


  	}else if(!(strcmp(selected_op, "range"))){
  		operation = RANGE;

  		if(input.range_lower > input.range_upper){
  			LOG(CRIT, "Please input Valid Range Values lower_range < upper_range");
  			exit(-1);
  		}

  	}else{
  		LOG(CRIT, "Invalid Operation Selected");
  		exit(-1);
  	}

  	/*Spwan threads*/
	spawn_threads(&input);

	/*Print BST*/
	printf("\nBST values KEY : VALUE\n");
	traverse_inorder(g_root);

	/*Freee allocated memory for key values*/
	free(node_values);

	return 0;
}
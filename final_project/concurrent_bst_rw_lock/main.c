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
	int create_low_index, create_upper_index, search_low_index, 
	search_upper_index, range_low_index, range_upper_index;
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
std::map<int, int> search_map
pthread_mutex_t search_map_lock;


pthread_mutex_t g_tree_lock;
bst_node *g_root = NULL;

void range_query(int range_lower, int range_upper, bst_node *root){
	
	if(root == NULL)
		return;

	pthread_mutex_lock(&root->lock);

	if(range_lower <= root->key && range_upper >= root->key){
		LOG(DBG, "Key found %d with value %d", root->key, root->value);
		pthread_mutex_lock(&range_map_lock);
		range_map.insert(pair<int, int>(root->key, root->value));
		pthread_mutex_unlock(&range_map_lock);
	}

	if(range_lower < root->key){		

		if(root->left == NULL){
			LOG(DBG, "Given Key not found or Node is Empty");
			pthread_mutex_unlock(&root->lock);
			return;
		}else{
			//pthread_mutex_lock(&root->left->lock);
			pthread_mutex_unlock(&root->lock);
			range_query(range_lower, range_upper, root->left);
			//root = root->left;
		}
	}

	if(range_upper > root->key){
		
		if(root->right == NULL){
			LOG(DBG, "Given Key not found or Node is Empty");
			pthread_mutex_unlock(&root->lock);
			return;
		}else{
			//pthread_mutex_lock(&root->right->lock);
			pthread_mutex_unlock(&root->lock);

			//root = root->right;
			range_query(range_lower, range_upper, root->right);			
		}
	}
}



void search_node(int key, int value, bst_node *root){

	LOG(DBG, "Searching a Node with key %d", key);

	pthread_mutex_lock(&g_tree_lock);
	
	if(g_root == NULL){

	 	LOG(DBG, "BST Empty or Node is Empty");
	 	pthread_mutex_unlock(&g_tree_lock);
	 	return;
	}	

	pthread_mutex_lock(&g_root->lock);
	pthread_mutex_unlock(&g_tree_lock);
	root = g_root;

	while(TRUE){		
		if(key < root->key){		

			if(root->left == NULL){
				LOG(DBG, "Given Key not found");
				pthread_mutex_unlock(&root->lock);
				return;
			}else{
				pthread_mutex_lock(&root->left->lock);
				pthread_mutex_unlock(&root->lock);
				root = root->left;
			}
		}else if(key > root->key){
			if(root->right == NULL){
				LOG(DBG, "Given Key not found");
				pthread_mutex_unlock(&root->lock);
				return;
			}else{
				pthread_mutex_lock(&root->right->lock);
				pthread_mutex_unlock(&root->lock);
				root = root->right;
			}
		}else{
			LOG(DBG,"Key Found %d with Value %d", root->key, root->value);
			pthread_mutex_unlock(&root->lock);
			return;
		}
	}
}


void insert_node(int key, int value, bst_node *root){

	LOG(DBG, "Inserting New Node with key %d", key);

	pthread_mutex_lock(&g_tree_lock);
	
	if(g_root == NULL){

	 	g_root = create_node(key, value);
	 	pthread_mutex_unlock(&g_tree_lock);
	 	return;
	}	

	pthread_mutex_lock(&g_root->lock);
	pthread_mutex_unlock(&g_tree_lock);
	root = g_root;

	while(TRUE){
		if(key < root->key){

			if(root->left == NULL){
				root->left = create_node(key, value);
				pthread_mutex_unlock(&root->lock);
				return;
			}else{
				pthread_mutex_lock(&root->left->lock);
				pthread_mutex_unlock(&root->lock);
				//insert_node(key, value, root->left);
				root = root->left;
			}
		}else if(key > root->key){
			if(root->right == NULL){
				root->right = create_node(key, value);
				pthread_mutex_unlock(&root->lock);
				return;
			}else{
				pthread_mutex_lock(&root->right->lock);
				pthread_mutex_unlock(&root->lock);
				//insert_node(key, value, root->right);
				root = root->right;
			}
		}else{
			LOG(DBG,"Duplicate Not Allowed");
			pthread_mutex_unlock(&root->lock);
			return;
		}
	}

}

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
	pthread_mutex_init(&node->lock, NULL);

	return node;
}

void *thread_function(void *params){

	thread_data_t *index = (thread_data_t*)(params);
	LOG(DBG, "Thread Range in the Thread function %d and %d", index->range_low_index, index->range_upper_index);

	for(int i = index->create_low_index; i <= index->create_upper_index; i++){
		insert_node(*(node_values + i), (*(node_values + i))*2, g_root);
	}

	switch(operation){

		case SEARCH:
			for(int i = index->search_low_index; i <= index->search_upper_index; i++){
				search_node(*(search_values + i), (*(node_values + i))*2, g_root);
			}
		break;

		case RANGE:

			range_query(index->range_low_index, index->range_upper_index, g_root);
			//range_query(4, 13, g_root);
		break;

		default:
		break;

	}

	return NULL;
}

void traverse_inorder(bst_node *root){

	if(root != NULL){
		traverse_inorder(root->left);
		printf("%d : %d\n", root->key, root->value);
		traverse_inorder(root->right);
	}
}


void print_map(std::map<int, int> &map){

    map<int, int>::iterator itr; 
    for (itr = map.begin(); itr != map.end(); ++itr) { 
       
         printf("\t %d", itr->first);
         printf("\t %d", itr->second);
    } 

}

void spawn_threads(user_input *input){

	int num_of_threads = input->num_of_threads;
	int num_create_nodes = input->create_nodes;
	int num_search_nodes = input->search_nodes;
	int range_lower = input-> range_lower;
	int range_upper = input->range_upper;
	int range = (range_upper - range_lower) + 1;

	/*Initialize global tree mutex*/
	pthread_mutex_init(&g_tree_lock, NULL);

	pthread_t threads[num_of_threads];
	
	int low = 0;
	thread_data_t a_thread_data_t[num_of_threads];


	/*Total size of the array is divided by the given number of threads 
	and each thread works on sorting the segment*/
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

			/*Total size of the array is divided by the given number of threads 
			and each thread works on searching the elements in the segment*/
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

			for(int i = 1; i <= num_of_threads; i++){
				a_thread_data_t[i].range_low_index = range_lower;

				if(i != num_of_threads){
					a_thread_data_t[i].range_upper_index = range_lower + range_seg - 1;
					range_lower += range_seg;
				}else{
					a_thread_data_t[i].range_upper_index = range_upper;

				}

				LOG(DBG, "Thread %d RANGE : %d and %d", i, a_thread_data_t[i].range_low_index, a_thread_data_t[i].range_upper_index);
			}

		}	
		break;

		default:
		break;
	}


	for(int i = 1; i <= num_of_threads; i++){

		if(pthread_create(&threads[i], NULL, thread_function, (void*)&a_thread_data_t[i])){
			LOG(CRIT, "Error while spawning a thread");
			exit(-1);
		}else{
			LOG(DBG, "Thread %d created successfully", i);
		}
	}

	for(int i = 1; i <= num_of_threads; i++){
		pthread_join(threads[i], NULL);
	}

	switch(operation){
		case SEARCH:
			print_map(search_map);
		break;

		case RANGE:
			print_map(range_map);
		break;
		
		default:
		break;

	}
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

	input.create_nodes = read_testfile(create_file, &node_values);

	if(!(strcmp(selected_op, "create"))){
		operation = CREATE;
		// input.create_nodes = read_testfile(create_file, &node_values);

		LOG(DBG, "Create Nodes Number: %d", input.create_nodes);

  	}else if(!(strcmp(selected_op, "search"))){
  		operation = SEARCH;

  		// input.create_nodes = read_testfile(create_file, &node_values);
  		input.search_nodes = read_testfile(search_file, &search_values);

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

	spawn_threads(&input);

	//traverse_inorder(g_root);

	free(node_values);
	free(search_values);
	//add cleanup for mutexes and BST

	return 0;
}
#include <stdlib.h>
#include <pthread.h>

/*Structure to represent a node of a binary search tree*/
typedef struct Node{

	int key, value;
	struct Node *left, *right;
	pthread_rwlock_t lock;
}bst_node;


typedef struct input{

	int count;
	int num_of_threads;
	int create_nodes;
	int search_nodes;
	int range_lower;
	int range_upper;
}user_input;

/*Function prototypes*/

void insert_node(int key, int value, bst_node *root);
void search_node(int key, int value, bst_node *root);
void range_query(int range_lower, int range_upper, bst_node *root);
bst_node* create_node(int key, int val);
void traverse_inorder(bst_node *root);
void spawn_threads(user_input *input);
int read_testfile(char *testfile, int **store_values);
void print_map(std::map<int, int> &map);



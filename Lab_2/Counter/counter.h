#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream> 
#include <atomic>
#include <pthread.h>
#include <bits/stdc++.h> 



const char *lock_names[] = {

	"tas",
	"ttas",
	"ticket",
	"mcs",
	"pthread"
};


const char *bar_names[] = {
	"sense",
	"pthread"
};

typedef struct{
	int tid;
}thread_data_t;


void spawn_threads();
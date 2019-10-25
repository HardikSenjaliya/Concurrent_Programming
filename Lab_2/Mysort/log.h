#include <iostream>

//#define ENABLE_LOGGING

typedef enum {

	NONE = 0,
	CRIT,
	WARN,
	LOG,
	DBG,

}LOG_LVL;

const char * log_level_strings [] = {
	"NONE",
	"CRIT",
	"WARN", 
	"NOTI",  
	"DEBG" 
};

#ifdef ENABLE_LOGGING
#define LOG(level, msg, ...) do{ \
		fprintf(stdout, "[%s] %s:%d: " msg " \n", log_level_strings[level], __FILE__, __LINE__, ##__VA_ARGS__);\
}while(0)	
#else
#define LOG(level, msg, ...)do{ \
		if(level == CRIT) \
			fprintf(stdout, "[%s] %s:%d: " msg " \n", log_level_strings[level], __FILE__, __LINE__, ##__VA_ARGS__);\
}while(0)
#endif


				
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

/**
 * @brief this function sorts the given array
 * @param *array pointer to the given array
 * @param arraySize size of the given array
 * @return void
 * @ref: https://www.geeksforgeeks.org/merge-sort/
 */

void merge_sort(int *array, int arraySize){

    int curr_size; /* For current size of subarrays to be merged curr_size varies from 1 to n/2*/
    int leftA_start; /* For picking starting index of left subarray to be merged*/

	/*Merge subarrays in bottom up manner. First merge subarrays of
	 size 1 to create sorted subarrays of size 2, then merge subarrays
	 of size 2 to create sorted subarrays of size 4, and so on.*/
    for (curr_size=1; curr_size<=arraySize-1; curr_size = 2*curr_size){
        /* Pick starting point of different subarrays of current size*/
        for (leftA_start=0; leftA_start<arraySize-1; leftA_start += 2*curr_size){
            /* Find ending point of left subarray. leftA_end+1 is starting
             point of right*/
            int leftA_end = leftA_start + curr_size - 1;

            int rightA_end = ((leftA_start + 2*curr_size - 1) < (arraySize-1)) ? (leftA_start + 2*curr_size - 1) : (arraySize-1);
            
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
    }

}

int main(int argc, char** argv){

	int i, rc = 0, count = 0;
	int *input_ints = NULL;
	char *input_file = NULL, *output_file = NULL;

	if(argc < 2){
		printf("Please provide atleast one argument\n");
		exit(-1);			
	}

	input_file = argv[1];
	//printf("Input File: %s\n", input_file);

	static struct option long_options[] = {

		{"name", no_argument, 0, 'n'},
		{"input_file", required_argument, 0, 1},
		{"output_file", required_argument, 0, 'o'},
		{0, 0, 0, 0}
	};
	
	/*Setting getopt_long for command line arguments --name and -o*/
	while((rc = getopt_long(argc, argv, ":o:", long_options, NULL)) != -1) {
		switch(rc){

			case 'n':
				printf("Hardik Senjaliya\n");
				exit(1);
				break;

			case 'o':
				output_file = optarg;
				//printf("Output File Name: %s\n", output_file);
				break;

			case ':':
        		printf("Missing Argument\n");
        		exit(-1);
                break;

            case '?':
            	printf("Invalid Option\n");
            	exit(-1);
            	break;

    	    default:
    	    	printf("Invalid Command line Argument\n");
            	exit(-1);
            	break;
		}
	}
	

	/*Open input file to read the data, as number of input integers
	is unknown realloc is used to dynamically allocate memory and handle 
	any number of input integers*/
    FILE *pFile = fopen(input_file, "r");
    if(pFile == NULL){
    	printf("Unable to open file\n");
    }
  	    
  	while (fscanf(pFile, "%d", &i) == 1){  
    	
  		if(input_ints == NULL){
  			input_ints = (int*)malloc(sizeof(int));
  			*input_ints = i;
  		}else{
  			count++;
  			input_ints = (int*)realloc(input_ints, sizeof(input_ints) * count);
  			*(input_ints + count) = i;
  		}    
  	}
  
  	/*Call the merge sort function to sort the input array*/
  	merge_sort(input_ints, count+1);
    
    /*Print the sorted data to the output file, if provided via command line
    argument else print to stdout*/
  	if(output_file == NULL){
    	for(int i = 0; i < count + 1; i++){
    		printf("%d\n", *(input_ints + i));
    	}
	}else{
    	FILE *pOFile = fopen(output_file, "w+");
    	if(pOFile == NULL){
    		printf("Unable to Open Output File\n");
    	}

    	for(int i = 0; i < count + 1; i++){
    		fprintf(pOFile, "%d\n", *(input_ints + i));
    	}

    	fclose(pOFile);
	}

  	fclose (pFile);  
  	free(input_ints);

   	return 0;
}
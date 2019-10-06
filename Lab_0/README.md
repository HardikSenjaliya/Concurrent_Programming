## ECEN 5033 Concurrent Programming
# LAB0 Report
**Why Merge Sort?**

As linked lists widely used data structure in most of the embedded systems. I selected merge sort for the following reasons.
* The worst-case complexity of quicksort is O(N^2) while for merge sort worst case and even average case has the same complexity of O(N log N), where N is the number of inputs
* Merge sort works the same with every dataset regardless of the size of the dataset. And merge sort works faster than quicksort when the data set is very large.
* Even though quicksort is preferred for sorting an array, I preferred mergesort because the merge sort is a widely used data structure in many embedded systems.

**Compilation Instructions**


* After downloading the submitted .zip file, extract and go into the directory.
* Use **make** command to build and create an executable named ​ mysort.

**Execution Instructions**

* ./mysort --name ​ prints student name
* ./mysort testfile.txt ​ sorts the data from the testfile.txt and prints the sorted data to stdout
* ./mysort testfile.txt -o output.txt ​ sorts the data from the testfile.txt and prints the sorted data to output.txt file.

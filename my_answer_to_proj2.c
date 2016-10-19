/* Memory manager
 *
 * Skeleton program written by Ben Rubinstein, May 2014
 *
 * Modifications by Tessa(Hyeri) Song, June 2014
 * Student number : 597952
 *
 * This program was designed to make small version of malloc-related functions,
 * which are malloc() and free(), creating a proper structure and delaring the 
 * structure variable globally.
 * When a user puts an invalid input line in stdin, this program gives an
 * appropriate error message on the screen and exits.
 * 
 * Algorithms are fun!
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define TOTALMEM	1048576
#define MAXVARS		1024
#define ERROR		-1
#define SUCCESS		1
#define LINELEN		5000
#define MAXLINES	100000
#define INPUT_INTS	'd'
#define INPUT_CHARS	'c'
#define FREE_DATA	'f'
#define INT_DELIM_S	","
#define INT_DELIM_C	','

typedef struct {
	char memory[TOTALMEM];	/* TOTALMEM bytes of memory */
	void *null;		/* first address will be  unusable */
	void *vars[MAXVARS];	/* MAXVARS variables, each at an address */
	size_t var_sizes[MAXVARS];	/* number of bytes per variable */
} mmanager_t;

mmanager_t manager;

/****************************************************************/

/* function prototypes */
int read_line(char *line, int maxlen);
void process_input_char(char *line, char *commands, void *stored[],
			int *storeLen, int numCommands);
void process_input_int(char *line, char *commands, void *stored[],
		       int *storeLen, int numCommands);
void process_free(char *line, char *commands, void *stored[],
		  int *storeLen, int numCommands);
int count_char(char c, char *s);
int parse_free(char* line, void* stored[], int numCommands);
int parse_integers(char *str, char *delim, int results[], int max_results);
void print_ints(int *intArray, size_t size);
void print_chars(char *charArray);
void print_memory(char isInt[MAXVARS]);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
int is_vacant(void *first, void* last);
void *select_address(size_t size);
int select_var(void);
void core_dump(char *filename_mem, char*filename_vars);


/****************************************************************/

/* orchestrate the entire program
 */
int
main(int argc, char *argv[]) {
    char line[LINELEN+1];
    char cmd[MAXLINES];
    void *stored[MAXLINES];
    int storeLen[MAXLINES];
    int i, numCmd = 0;

    /* initialise our very own NULL */
    manager.null = manager.memory;

    /* process input commands that make use of memory management */
    while (numCmd<MAXLINES && read_line(line,LINELEN)) {
    	if (strlen(line) < 2) {
	    fprintf(stderr, "Invalid line %s\n", line);
  	    return EXIT_FAILURE;
	}
	
	if (line[0] == INPUT_CHARS) {
	    process_input_char(line, cmd, stored, storeLen, numCmd);
	} else if (line[0] == INPUT_INTS) {
 	    process_input_int(line, cmd, stored, storeLen, numCmd);
	} else if (line[0] == FREE_DATA) {
	    process_free(line, cmd, stored, storeLen, numCmd);
	} else {
	    fprintf(stderr, "Invalid input %c.\n", line[0]);
	    return EXIT_FAILURE;
	}
	
	numCmd++;
    }

    /* print out what we are left with
     * after creating variables, deleting some, creating more, ...
     */
    printf("Cmd#\tOffset\tValue\n");
    printf("====\t======\t=====\n");
    for (i=0; i<numCmd; i++) {
	if (storeLen[i] > 0) {
	    printf("%d\t%d\t", i, (int)((char*)stored[i]-manager.memory));
	    if (cmd[i] == INPUT_CHARS) {
		print_chars((char*)stored[i]);
	    } else {
		print_ints((int*)stored[i], storeLen[i]);
	    }
	}
    }
    /* call core_dump */
    core_dump("core_mem", "core_vars");
    return 0;
}

/****************************************************************/

/* read in a line of input
 */
int
read_line(char *line, int maxlen) {
    int n = 0;
    int oversize = 0;
    int c;
    while (((c=getchar())!=EOF) && (c!='\n')) {
	if (n < maxlen) {
	    line[n++] = c;
	}
	else {
	    oversize++;
	}
    }
    line[n] = '\0';
    if (oversize > 0) {
	fprintf(stderr, "Warning! %d over limit. Line truncated.\n", 
	    oversize);
	}
    return ((n>0) || (c!=EOF));
}

/****************************************************************/

/* process an input-char command from stdin by storing the string
 */
void
process_input_char(char *line, char *commands, void *stored[], 
		   int *storeLen, int numCommands) {
    size_t lineLen = strlen(line);
    commands[numCommands] = line[0];
    stored[numCommands] = mm_malloc(lineLen);
    assert(stored[numCommands] != manager.null);
    strcpy(stored[numCommands], line+1);
    storeLen[numCommands] = lineLen;
}

/****************************************************************/

/* process an input-int command from stdin by storing the ints
 */
void
process_input_int(char *line, char *commands, void *stored[], 
		  int *storeLen, int numCommands) {
    int intsLen = count_char(INT_DELIM_C, line+1) + 1;
    size_t size = sizeof(intsLen) * intsLen;
    commands[numCommands] = line[0];
    stored[numCommands] = mm_malloc(size);
    assert(stored[numCommands] != manager.null);
    parse_integers(line+1, INT_DELIM_S, stored[numCommands], intsLen);
    storeLen[numCommands] = intsLen;
}

/****************************************************************/

/* process a free command from stdin
 */
void
process_free(char *line, char *commands, void *stored[], 
	     int *storeLen, int numCommands) {

    int f_num;
    /* check if it is a valid command */
    f_num = parse_free(line+1, stored, numCommands);
    
    commands[numCommands] = line[0];
    stored[numCommands] = manager.null;
    storeLen[numCommands] = 0;

    /* call mm_free to free the allocated memory */
    mm_free(stored[f_num-1]);
    
    stored[f_num-1] = manager.null;
    storeLen[f_num-1] = 0;	
}

/****************************************************************/

/* Count the number of occurences of a char in a string
 */
int
count_char(char c, char *s) {
    int count = 0;
    if (!s) {
	return count;
    }
    while (*s != '\0') {
	if (*(s++) == c) {
	    count++;
	}
    }
    return count;
}

/****************************************************************/

/* check if the number that 'f' command is followed by is valid or not.
 * If not valid, an error message appears on screen and the program exits. 
 */
int
parse_free(char* line, void* stored[], int numCommands){
    int f_num = atoi(line);
    int i;
    
    if(f_num <= 0){
        fprintf(stderr, "Not available.\n");
        exit(EXIT_FAILURE);
    }
    
    if(f_num > numCommands){
    	fprintf(stderr, "The number should be less or equal than %d.\n"
    		, numCommands);
    	exit(EXIT_FAILURE);
    }
    
    if(stored[f_num-1] == manager.null){
    	fprintf(stderr, "The command was alreadly freed.\n");
    	exit(EXIT_FAILURE);
    }
    
    for(i = 0; i<strlen(line); i++){
        if('0'>line[i] || '9'<line[i]){
            fprintf(stderr,
            	    "Neither spaces nor chars other than numbers allowed.\n");
            exit(EXIT_FAILURE);
        }
    }
    return f_num;
}

/****************************************************************/

/* parse string for a delimited-list of positive integers.
 * Returns number of ints parsed. If invalid input is detected
 * or if more than max_results ints are parsed, execution
 * will halt. Note! str will be modified as a side-effect of
 * running this function: delims replaced by \0
 */
int
parse_integers(char *str, char *delim, int results[], int max_results) {
    int num_results = 0;
    int num;
    char *token;
    token = strtok(str, delim);
    while (token != NULL) {
	if ((num=atoi(token)) <= 0) {
	    fprintf(stderr, "Non-int %s.\n", token);
	    exit(EXIT_FAILURE);
	}
        if (num_results >= max_results) {
	    fprintf(stderr, "Parsing too many ints.\n");
	    exit(EXIT_FAILURE);
	}
	results[num_results++] = num;
	token = strtok(NULL, delim);
    }
    return num_results;
}

/****************************************************************/

/* print an array of ints
 */
void
print_ints(int *intArray, size_t size) {
    int i;
    assert(size > 0);
    printf("ints: %d", intArray[0]);
    for (i=1; i<size; i++) {
	printf(", %d", intArray[i]);
    }
    putchar('\n');
}

/****************************************************************/

/* print an array of chars
 */
void
print_chars(char *charArray) {
    printf("chars: %s\n", charArray);
}

/****************************************************************/

/* prints what is stored in memory given an array specifying
 * how previously allocated variables should be interpreted
 * (as int or char arrays)
 */
void
print_memory(char isInt[MAXVARS]) {
    void *start;
    int i, num_bytes;
    for (i=0; i<MAXVARS && manager.var_sizes[i]>0; i++) {
	num_bytes = manager.var_sizes[i];
	start = manager.vars[i];
	if (isInt[i]) {
	    print_ints((int*)start, num_bytes/sizeof(i));
	} else {
	    print_chars((char*)start);
	}
    }
}

/****************************************************************/

/* perform the similar task to what malloc funtion does.
 * allocate a requested memory using several functions and return the address.
 * if it fails, returns manager.null 
 */
void *
mm_malloc(size_t size) {
    int idx;
    void* start = select_address(size);
	
    if (start == manager.null){
	return manager.null;
    }
	
    idx = select_var();
	
    if(idx == ERROR){
	return manager.null;
    }
    
    /* all conditions satisfied. allocate memory. */	
    manager.var_sizes[idx] = size;
    manager.vars[idx] = start;
    return start;
}

/****************************************************************/

/* check if the address ptr has been allocated as the start of an allocated 
 * block and free it by updating manager.vars and manager.var_sizes.
 */
void 
mm_free(void *ptr){
    int i;
    for(i = 0; i<MAXVARS; i++){
	if(ptr == manager.vars[i]){
	    manager.vars[i] = manager.null;
	    manager.var_sizes[i] = 0;
	    return;
	}
    }
}

/****************************************************************/

/* get two pointer arguments, which indicate the first and the last address 
 * respectively, and check if the memory between the addresses are valid.
 */
int
is_vacant(void *first, void *last){
    int i;
    char* first_ch = first;
    char* last_ch = last;	
    
    /* check validity one by one */
    while(last_ch-first_ch > -1){
    	for(i = 0;i<MAXVARS; i++){
       	    if(manager.var_sizes[i] > 0 && manager.vars[i] != manager.null){
       	    	if ((char *)manager.vars[i] <= last_ch && last_ch
		    < ((char *)manager.vars[i] + manager.var_sizes[i])){
		    return 0;
		}
	    }
	}
	last_ch-=1;
    }
    return 1;
}

/****************************************************************/

/* select the earliest available address which can accommodate the passed size.
 */
void *
select_address(size_t size){
    int i;
    for(i = 1; i<= TOTALMEM-size ; i++){
	if(is_vacant(manager.memory+i, manager.memory+(i+size-1))){
	    return manager.memory+i;
	}
    }
    return manager.null;
}

/****************************************************************/

/* select the earliest available index into manager.vars which is not assigned.
 */
int 
select_var(void){
    int i;
    for(i = 0; i<MAXVARS ;i++){
	if(manager.var_sizes[i]<=0 || manager.vars[i] == manager.null){
	    return i;
	}
    }
    return ERROR;
}

/****************************************************************/

/* at the end of the main function, all the resources stored in manger.memory
 * is written to disk to the binary file with name filename_mem.
 * some useful information on the stored resources are written to the
 * text file named filename_vars (an integer 
 * offset, an integer correspoinding to the size in manager.var_sizes).
 */
void core_dump(char *filename_mem, char* filename_vars){
	
    /* open two filestreams with "w" */	
    FILE* mem_fptr = fopen(filename_mem, "w"); 
    FILE* vars_fptr = fopen(filename_vars, "w");
    int i;

    fwrite(manager.memory, sizeof(*(manager.memory)), TOTALMEM, mem_fptr);
	
    for(i = 0; i<MAXVARS; i++){
	if(manager.var_sizes[i] > 0 && manager.vars != manager.null){
	    fprintf(vars_fptr,"%d\t",
	    	    (int)((char*)manager.vars[i]-manager.memory));
	    fprintf(vars_fptr,"%d\n", (int)manager.var_sizes[i]);
	}
    }
    fclose(mem_fptr);
    fclose(vars_fptr);
}

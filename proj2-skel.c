/* Memory manager
 *
 * Skeleton program written by Ben Rubinstein, May 2014
 *
 * Modifications by XXXXX, May 2014
 * (Add your name and student number)
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
int parse_integers(char *str, char *delim, int results[], int max_results);
void print_ints(int *intArray, size_t size);
void print_chars(char *charArray);
void print_memory(char isInt[MAXVARS]);
void *mm_malloc(size_t size);

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
			printf("%d\t%d\t", i, (char*)stored[i]-manager.memory);
			if (cmd[i] == INPUT_CHARS) {
				print_chars((char*)stored[i]);
			} else {
				print_ints((int*)stored[i], storeLen[i]);
			}
		}
	}
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
	int i;

	/* Make note of this free command, so that at the end of main
	 * we can print out a trace of everything we've done including
	 * this free. Unlike when we add things, we're not actually 
	 * storing anything associated with the free command
	 */
	commands[numCommands] = line[0];
	stored[numCommands] = manager.null;

	/* CHANGE!!! This function should free the variable stored
	 * from a previous input command, as specified by the syntax
	 * of the 'f' input command. But right now it doesn't do
	 * this, as there's no mm_free(). Right now, the program 
	 * stores all data at manager.memory+1. So, when given an input
	 * command 'f' that that's the only place to free. Moreoever
	 * we only ever use manager.vars[0], so that's what we have
	 * to wipe! When you fix this, you'll have to figure out
	 * which variable in stored[] is being free'd, free it, then
	 * also notify the manager that the space is no longer used.
	 */
	for (i=0; i<numCommands; i++) {
		/* Note that what we had previously stored from
		 * input is no longer
		 */
		stored[i] = manager.null;
		storeLen[i] = 0;
		/* Now actually notify the memory manager that
		 * we no longer need that memory
		 */
		manager.vars[0] = manager.null;
		manager.var_sizes[0] = 0;
	}
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

/* a bad version of malloc! change!!!
 */
void *
mm_malloc(size_t size) {
	/* Goldfish memory: whenever memory is requested,
	 * we chuck out anything we had previously stored
	 * first. There's only ever zero or one things in 
	 * memory!
	 */
	manager.var_sizes[0] = size;
	manager.vars[0] = manager.memory + 1;
	return manager.vars[0];
}


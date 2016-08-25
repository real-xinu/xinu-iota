#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

/* ****************************************************
 * Example Run:										  *
 * gcc -o sort sort.c 								  *
 * ./sort test_dir									  *
 *													  *
 * Program that takes a directory name (that contains *
 * Topology files) as an input and outputs the file   *
 * names in sorted order. 							  *
 * ****************************************************/

/* Compare function by string comparison */
int comp_func(const void * a, const void * b)
{
	const char ** elem_a = (const char **)a;
	const char ** elem_b = (const char **)b;

	return strcmp(*elem_a, *elem_b);
}

/* Compare function by length */
int comp_func_len(const void * a, const void * b)
{
	const char ** elem_a = (const char **)a;
	const char ** elem_b = (const char **)b;

	return strlen(*elem_a) - strlen(*elem_b);
}

/* Sort function for sorting elements lexicographically 
 * first and then by length */
void sort(char ** list, int n)
{
	int i = 0;

	/* Sort Lexicographically */
	qsort(list, n, sizeof(char *), comp_func);

	/* Sort by length of string */
	qsort(list, n, sizeof(char *), comp_func_len);
}

/* Read a directory containing topology files and 
 * create a list 
 * @param name - name of a valid directory 
 * @param n - pointer that will store the number of files after the function. */
char ** read_dir(char * name, int * n)
{
	/* Sanity check */
	if(name == NULL)
	{
		*n = -1;
		return NULL;
	}

	DIR * dir;
	struct dirent * dp;
	int count = 0, size = 10;
	
	/* Open the directory */
	if((dir = opendir(name)) == NULL)
	{
		fprintf(stderr, "Cannot open %s.\n", name);
		*n = -1;
		return NULL;
	}	

	/* Allocate memory for the list */
	char ** list = (char **)calloc(10, sizeof(char *));
	for(count = 0; count < 10; count++)
		list[count] = NULL;
	count = 0;

	/* Loop through the contents of the directory */
	while((dp = readdir(dir)) != NULL)
	{
		/* If the file is not current directory or an upper level directory or if the
		 * file is not a regular file then continue looping through */
		if(!strcmp(".", dp->d_name) || !strcmp("..", dp->d_name) || dp->d_type != DT_REG || strncmp("Topology-K", dp->d_name, 10) )
			continue;

		/* Reallocating memory and copying contents if needed */
		if( count == size )
		{
			size *= 2;
			list = (char **)realloc(list, size * sizeof(char *));
		}

		/* Allocate memory and copy the file name into the list
		 * and increment the number of files detected */
		list[count] = (char *)calloc(strlen(dp->d_name), sizeof(char));
		strcpy(list[count], dp->d_name);
		count++;
	}

	/* Close the directory, copy the number of files and return
	 * the list */
	closedir(dir);
	*n = count;
	return list;
}

/* Driver function for testing */
void sort_print_dir(char * name)
{
	int n;
	char ** list = read_dir(name, &n);
	if(list == NULL)
		return;
	
	sort(list, n);

	int i = 0;
	for(i = 0; i < n; i++)
		fprintf(stderr, "%s ", *(list + i));

	fprintf(stderr, "\n");
}

void main(int argc, char ** argv)
{
	sort_print_dir(argv[1]);
}

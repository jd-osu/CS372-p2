/************************************************
 * [program name]
 * Author:
 * CS372
 * October 27, 2018
 * This program ...
 * *********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * *********************************************/
void get_handle(char* handle)
{
  char *input;
  int valid_input = 0;

  char space_str[] = " ";

  size_t input_size = 50;

  // allocate memory for input string
  input = (char *)malloc(input_size * sizeof(char));
  if (input == NULL)
  {
    fprintf(stderr, "Unable to allocate");
    perror("Error in get_choice");
    exit(1);
  }

  // until a valid input is received...
  while (valid_input == 0)
  {
    // prompt the user
    printf("Enter handle: ");
    getline(&input, &input_size, stdin);

    if (input[strlen(input)-1] == '\n')
      input[strlen(input)-1] = 0;

    if ((strstr(input, space_str) == NULL) && (strlen(input) > 0) && (strlen(input) <= 10))
      valid_input = 1;
    else
      printf("Handle must be 1-10 characters with no spaces.\n");
  }

  strcpy(handle, input);

  // free dynamically allocated memory
  free(input);
}



int main(void) {
	char handle[11];

	get_handle(handle);

	printf("Handle is: %s\n", handle);

	return EXIT_SUCCESS;
}

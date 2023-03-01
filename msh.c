// The MIT License (MIT)
// 
// Copyright (c) 2016 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size
#define MAX_HISTORY 15
#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports four arguments

char history[MAX_HISTORY][MAX_COMMAND_SIZE];
int history_size = 0;
pid_t pids[MAX_HISTORY] = {-1, -1, -1, -1, -1 , -1, -1, -1, -1, -1 , -1, -1, -1, -1, -1};
int pids_size = 0;

int main()
{
  char * command_string = (char*) malloc( MAX_COMMAND_SIZE ); //string with size 255

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    // Check if cmd starts with !. If so tokenize and take value after !(n) and the save the value stored at history[n] in cmd string.
    if(command_string[0] == '!')
    {
      char* token = strtok(command_string, "!");
      int value = atoi(token);

      if(value < history_size)
      {
        command_string = history[value];
      }
      else
      {
        printf("Command not in history.\n");
        continue;
      }
    }

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup( command_string );                

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE ); //all input gets parsed into token array
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL; 
      }
        token_count++;
    }

    //*******************************************************************
    
    // If blank is entered, continue.
    if(token[0] == NULL) {
      continue;
    }

    // Pid used to keep pid after parent terminates child
    pid_t saved_pid;


    // If history is full, move all elements in array to left by one, then add current command to last element in array.
    if(history_size == MAX_HISTORY)
    {
      for(int i = 0; i < MAX_HISTORY; i++)
      {
        strcpy(history[i], history[i+1]);
      }
      strcpy(history[history_size-1], command_string);
    }
    // If history is not full, store command in history array. Once history is full, don't increase size.
    else
    {
      strcpy(history[history_size], command_string);
      history_size++;
    }


    // Works identical to history above
    if(pids_size == MAX_HISTORY)
    {
      for(int i = 0; i < MAX_HISTORY; i++)
      {
        pids[i] = pids[i+1];
      }
      pids[pids_size-1] = saved_pid;
    }


    // Set saved pids to -1 for built in commands. Will change to actual pid after forking.
    saved_pid = -1;

    // If command is built in - called from parent

    // If user enters command quit or exit, terminate the process.
    if(!strcmp(token[0], "quit") || !strcmp(token[0], "exit")) 
    {
      exit(0);
    }

    // Change directory to path requested by user.
    else if(!strcmp(token[0], "cd"))
    {
      chdir(token[1]); 
    }
    
    // Print history of cmds or cmds with pids if -p argument is entered.
    else if(!strcmp(token[0], "history"))
    { 
      if(token[1] == NULL)
      {
        for(int i = 0; i<history_size; i++)
        {
          printf("%d. %s", i, history[i]);
        }
      }

      else if(!strcmp(token[1], "-p"))
      {
        for(int i = 0; i<history_size; i++)
        {
          printf("%d. [%d] %s", i, pids[i], history[i]);
        }
      }
    }

    // Else if command is forked - called from child
    else
    {
      pid_t child_pid = fork(); // create new child process

      if(child_pid == 0) // child created successfully
      {
        int ret = execvp(token[0], &token[0]);
        if(ret == -1) //exec failed or command not found
        {
          printf("%s: Command not found.\n", token[0]);
        }
        exit(0);
      }

      else if(child_pid == -1) //error occurred
      {
        exit(0);
      }

      else // Wait for child process to terminate
      {
        int status;
        wait(&status);
        saved_pid = child_pid;
      }
    }


    // Works identical to history above.
    if(pids_size != MAX_HISTORY)
    {
      pids[pids_size] = saved_pid;
      pids_size++;
    }


    // Cleanup allocated memory
    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      if( token[i] != NULL )
      {
        free( token[i] );
      }
    }

    free( head_ptr );
  }

  free( command_string );

  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}

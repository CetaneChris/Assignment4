// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
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

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

typedef int bool;
#define true 1
#define false 0

struct fat_details{
	int BPB_BytesPerSec;
	int BPB_SecPerClus;
	int BPB_RsvdSecCnt;
	int BPB_NumFATS;
	int BPB_FATSz32;
};

int main()
{
	char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

	bool state = false;
	bool been_closed = false;
	FILE *img;
	char buffer[100];
	int  i;
	struct fat_details* FAT = malloc(sizeof(struct fat_details));

	while( 1 ){
		// Print out the msh prompt
		printf ("mfs> ");

		// Read the command from the commandline.  The
		// maximum command that will be read is MAX_COMMAND_SIZE
		// This while command will wait here until the user
		// inputs something since fgets returns NULL when there
		// is no input
		while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

		/* Parse input */
		char *token[MAX_NUM_ARGUMENTS];

		int   token_count = 0;

		// Pointer to point to the token
		// parsed by strsep
		char *arg_ptr;

		char *working_str  = strdup( cmd_str ); 

		// we are going to move the working_str pointer so
		// keep track of its original value so we can deallocate
		// the correct amount at the end
		char *working_root = working_str;

		// Tokenize the input stringswith whitespace used as the delimiter
		while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) && (token_count<MAX_NUM_ARGUMENTS)){
			token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
			if( strlen( token[token_count] ) == 0 )
				token[token_count] = NULL;
			token_count++;
		}

		// Now print the tokenized input as a debug check
		// TODO Remove this code and replace with your shell functionality

		if(!strcmp(token[0], "exit") || !strcmp(token[0], "quit")){
			free(working_root);             //Checking for exit condition, freeing used data, and exiting
			if(state)
				fclose(img);
			break;
		}else if(strcmp(token[0],"open") && been_closed && !state){
			fprintf(stderr, "Error: File system image must be opened first\n");
//			free(working_root);
//			continue;
		}else if(strcmp(token[0],"open") && !state){
			fprintf(stderr, "Error: File system not open\n");
//			free(working_root);
//			continue;
		}else if(!strcmp(token[0],"open")){
			if(state)		// If a file is already open
				fprintf(stderr,"Error: File system image already open\n");
			else if(token[1] != NULL){	// Ensuring a file was selected
				img   = fopen(token[1], "r");	// Open file in read only mode
				if(img == NULL)		// If file not found, print error
					fprintf(stderr,"Error: File system image not found\n");
				else{
					state = true;		// Set program state to open

					// Ensure buffer location
					fseek(img,11,SEEK_SET);

					fread(&FAT->BPB_BytesPerSec, 2, 1, img);	// Retrieve attributes for 'info'
					fread(&FAT->BPB_SecPerClus, 1, 1, img);
					fread(&FAT->BPB_RsvdSecCnt, 2, 1, img);
					fread(&FAT->BPB_NumFATS, 1, 1, img);

					fseek(img,36,SEEK_SET);

					fread(&FAT->BPB_FATSz32, 4, 1, img);
				}
			}
		}else if(!strcmp(token[0],"close")){
			if(state){	// If file open, then close it
				state = false;		// Set program state to closed
				been_closed = false;	// Program knows that 'close' has been ran
				fclose(img);		// Close the file
			}else		// Else, print error
				fprintf(stderr,"Error: File system not open\n");
		}else if(!strcmp(token[0],"info")){
			printf("BPB_BytesPerSec = %d\n", FAT->BPB_BytesPerSec);
			printf("BPB_BytesPerSec = %x\n\n", FAT->BPB_BytesPerSec);

			printf("BPB_SecPerClus  = %d\n", FAT->BPB_SecPerClus);
			printf("BPB_SecPerClus  = %x\n\n", FAT->BPB_SecPerClus);

			printf("BPB_RsvdSecCnt  = %d\n", FAT->BPB_RsvdSecCnt);
			printf("BPB_RsvdSecCnt  = %x\n\n", FAT->BPB_RsvdSecCnt);

			printf("BPB_NumFATS     = %d\n", FAT->BPB_NumFATS);
			printf("BPB_NumFATS     = %x\n\n", FAT->BPB_NumFATS);

			printf("BPB_FATSz32     = %d\n", FAT->BPB_FATSz32);
			printf("BPB_FATSz32     = %x\n\n", FAT->BPB_FATSz32);
		}else if(!strcmp(token[0],"stat")){
			//something
		}else if(!strcmp(token[0],"get")){
			//something
		}else if(!strcmp(token[0],"cd")){
			//something
		}else if(!strcmp(token[0],"ls")){
			//
		}else if(!strcmp(token[0],"read")){
			// Move to position token[1] and begin reading
			char* buffer;
		}else if(!strcmp(token[0],"volume")){
			//something
		}

		free(working_root);

	}
	return 0;
}
